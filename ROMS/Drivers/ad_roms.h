      MODULE roms_kernel_mod
!
!git $Id$
!svn $Id: ad_roms.h 1064 2021-05-10 19:55:56Z arango $
!================================================== Hernan G. Arango ===
!  Copyright (c) 2002-2021 The ROMS/TOMS Group                         !
!    Licensed under a MIT/X style license                              !
!    See License_ROMS.txt                                              !
!=======================================================================
!                                                                      !
!  ROMS/TOMS Adjoint Adjoint Model Driver:                             !
!                                                                      !
!  This driver executes ROMS/TOMS generic adjoint model.  It controls  !
!  the initialization, time-stepping, and finalization of the adjoint  !
!  model execution following ESMF conventions:                         !
!                                                                      !
!     ROMS_initialize                                                  !
!     ROMS_run                                                         !
!     ROMS_finalize                                                    !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_parallel
      USE mod_iounits
      USE mod_ncparam
      USE mod_scalars
!
      USE close_io_mod,      ONLY : close_inp, close_out
      USE inp_par_mod,       ONLY : inp_par
#ifdef MCT_LIB
# ifdef ATM_COUPLING
      USE ocean_coupler_mod, ONLY : initialize_ocn2atm_coupling
# endif
# ifdef WAV_COUPLING
      USE ocean_coupler_mod, ONLY : initialize_ocn2wav_coupling
# endif
#endif
      USE strings_mod,       ONLY : FoundError
      USE wrt_rst_mod,       ONLY : wrt_rst
!
      implicit none
!
      PUBLIC  :: ROMS_initialize
      PUBLIC  :: ROMS_run
      PUBLIC  :: ROMS_finalize
!
      CONTAINS
!
      SUBROUTINE ROMS_initialize (first, mpiCOMM)
!
!=======================================================================
!                                                                      !
!  This routine allocates and initializes ROMS/TOMS state variables    !
!  and internal and external parameters.                               !
!                                                                      !
!=======================================================================
!
!  Imported variable declarations.
!
      logical, intent(inout) :: first
!
      integer, intent(in), optional :: mpiCOMM
!
!  Local variable declarations.
!
      logical :: allocate_vars = .TRUE.
!
#ifdef DISTRIBUTE
      integer :: MyError, MySize
#endif
      integer :: chunk_size, ng, thread
#ifdef _OPENMP
      integer :: my_threadnum
#endif
!
      character (len=*), parameter :: MyFile =                          &
     &  __FILE__//", ROMS_initialize"

#ifdef DISTRIBUTE
!
!-----------------------------------------------------------------------
!  Set distribute-memory (mpi) world communictor.
!-----------------------------------------------------------------------
!
      IF (PRESENT(mpiCOMM)) THEN
        OCN_COMM_WORLD=mpiCOMM
      ELSE
        OCN_COMM_WORLD=MPI_COMM_WORLD
      END IF
      CALL mpi_comm_rank (OCN_COMM_WORLD, MyRank, MyError)
      CALL mpi_comm_size (OCN_COMM_WORLD, MySize, MyError)
#endif
!
!-----------------------------------------------------------------------
!  On first pass, initialize model parameters a variables for all
!  nested/composed grids.  Notice that the logical switch "first"
!  is used to allow multiple calls to this routine during ensemble
!  configurations.
!-----------------------------------------------------------------------
!
      IF (first) THEN
        first=.FALSE.
!
!  Initialize parallel control switches. These scalars switches are
!  independent from standard input parameters.
!
        CALL initialize_parallel
!
!  Read in model tunable parameters from standard input. Allocate and
!  initialize variables in several modules after the number of nested
!  grids and dimension parameters are known.
!
        CALL inp_par (iADM)
        IF (FoundError(exit_flag, NoError, __LINE__, MyFile)) RETURN
!
!  Set domain decomposition tile partition range.  This range is
!  computed only once since the "first_tile" and "last_tile" values
!  are private for each parallel thread/node.
!
#if defined _OPENMP
      MyThread=my_threadnum()
#elif defined DISTRIBUTE
      MyThread=MyRank
#else
      MyThread=0
#endif
      DO ng=1,Ngrids
        chunk_size=(NtileX(ng)*NtileE(ng)+numthreads-1)/numthreads
        first_tile(ng)=MyThread*chunk_size
        last_tile (ng)=first_tile(ng)+chunk_size-1
      END DO
!
!  Initialize internal wall clocks. Notice that the timings does not
!  includes processing standard input because several parameters are
!  needed to allocate clock variables.
!
        IF (Master) THEN
          WRITE (stdout,10)
 10       FORMAT (/,' Process Information:',/)
        END IF
!
        DO ng=1,Ngrids
          DO thread=THREAD_RANGE
            CALL wclock_on (ng, iADM, 0, __LINE__, MyFile)
          END DO
        END DO
!
!  Allocate and initialize modules variables.
!
        CALL mod_arrays (allocate_vars)

      END IF

#if defined MCT_LIB && (defined ATM_COUPLING || defined WAV_COUPLING)
!
!-----------------------------------------------------------------------
!  Initialize coupling streams between model(s).
!-----------------------------------------------------------------------
!
      DO ng=1,Ngrids
# ifdef ATM_COUPLING
        CALL initialize_ocn2atm_coupling (ng, MyRank)
# endif
# ifdef WAV_COUPLING
        CALL initialize_ocn2wav_coupling (ng, MyRank)
# endif
      END DO
#endif
!
!-----------------------------------------------------------------------
!  Initialize adjoint model state variables over all nested grids, if
!  applicable.
!-----------------------------------------------------------------------

#ifdef FORWARD_FLUXES
!
!  Set the BLK structure to contain the nonlinear model surface fluxes
!  needed by the tangent linear and adjoint models. Also, set switches
!  to process that structure in routine "check_multifile". Notice that
!  it is possible to split the solution into multiple NetCDF files to
!  reduce their size.
!
!  The switch LreadFRC is deactivated because all the atmospheric
!  forcing, including shortwave radiation, is read from the NLM
!  surface fluxes or is assigned during ESM coupling.  Such fluxes
!  are available from the QCK structure. There is no need for reading
!  and processing from the FRC structure input forcing-files.
!
      CALL edit_multifile ('QCK2BLK')
      IF (FoundError(exit_flag, NoError, __LINE__, MyFile)) RETURN
      DO ng=1,Ngrids
        LreadBLK(ng)=.TRUE.
        LreadFRC(ng)=.FALSE.
      END DO
#endif
!
!  Initialize adjoint model.
!
      Lstiffness=.FALSE.
      DO ng=1,Ngrids
        LreadFWD(ng)=.TRUE.
        CALL ad_initial (ng)
        IF (FoundError(exit_flag, NoError, __LINE__, MyFile)) RETURN
      END DO
!
!  Initialize run or ensemble counter.
!
      Nrun=1
!
!  Activate adjoint output.
!
      DO ng=1,Ngrids
        LdefADJ(ng)=.TRUE.
        LwrtADJ(ng)=.TRUE.
        LcycleADJ(ng)=.FALSE.
      END DO
!
      RETURN
      END SUBROUTINE ROMS_initialize
!
      SUBROUTINE ROMS_run (RunInterval)
!
!=======================================================================
!                                                                      !
!  This routine runs ROMS/TOMS adjoint model backwards for the         !
!  specified time interval (seconds), RunInterval.                     !
!                                                                      !
!=======================================================================
!
!  Imported variable declarations.
!
      real(dp), intent(in) :: RunInterval            ! seconds
!
!  Local variable declarations.
!
      integer :: ng
!
      character (len=*), parameter :: MyFile =                          &
     &  __FILE__//", ROMS_run"
!
!-----------------------------------------------------------------------
!  Time-step adjoint model over all nested grids, if applicable.
!-----------------------------------------------------------------------
!
      DO ng=1,Ngrids
        IF (Master) THEN
          WRITE (stdout,10) 'AD', ng, ntstart(ng), ntend(ng)
        END IF
      END DO
!
#ifdef SOLVE3D
      CALL ad_main3d (RunInterval)
#else
      CALL ad_main2d (RunInterval)
#endif

      IF (FoundError(exit_flag, NoError, __LINE__, MyFile)) RETURN
!
 10   FORMAT (/,1x,a,1x,'ROMS/TOMS: started time-stepping:',            &
     &        ' (Grid: ',i2.2,' TimeSteps: ',i8.8,' - ',i8.8,')',/)
!
      RETURN
      END SUBROUTINE ROMS_run
!
      SUBROUTINE ROMS_finalize
!
!=======================================================================
!                                                                      !
!  This routine terminates ROMS/TOMS adjoint model execution.          !
!                                                                      !
!=======================================================================
!
!  Local variable declarations.
!
      integer :: Fcount, ng, thread
!
      character (len=*), parameter :: MyFile =                          &
     &  __FILE__//", ROMS_finalize"
!
!-----------------------------------------------------------------------
!  If blowing-up, save latest model state into RESTART NetCDF file.
!-----------------------------------------------------------------------
!
!  If cycling restart records, write solution into the next record.
!
      IF (exit_flag.eq.1) THEN
        DO ng=1,Ngrids
          IF (LwrtRST(ng)) THEN
            IF (Master) WRITE (stdout,10)
 10         FORMAT (/,' Blowing-up: Saving latest model state into ',   &
     &                ' RESTART file',/)
            Fcount=RST(ng)%load
            IF (LcycleRST(ng).and.(RST(ng)%Nrec(Fcount).ge.2)) THEN
              RST(ng)%Rindex=2
              LcycleRST(ng)=.FALSE.
            END IF
            blowup=exit_flag
            exit_flag=NoError
#ifdef DISTRIBUTE
            CALL wrt_rst (ng, MyRank)
#else
            CALL wrt_rst (ng, -1)
#endif
          END IF
        END DO
      END IF
!
!-----------------------------------------------------------------------
!  Stop model and time profiling clocks, report memory requirements, and
!  close output NetCDF files.
!-----------------------------------------------------------------------
!
!  Stop time clocks.
!
      IF (Master) THEN
        WRITE (stdout,20)
 20     FORMAT (/,'Elapsed wall CPU time for each process (seconds):',/)
      END IF
!
      DO ng=1,Ngrids
        DO thread=THREAD_RANGE
          CALL wclock_off (ng, iADM, 0, __LINE__, MyFile)
        END DO
      END DO
!
!  Report dynamic memory and automatic memory requirements.
!
      CALL memory
!
!  Close IO files.
!
      DO ng=1,Ngrids
        CALL close_inp (ng, iADM)
      END DO
      CALL close_out
!
      RETURN
      END SUBROUTINE ROMS_finalize

      END MODULE roms_kernel_mod