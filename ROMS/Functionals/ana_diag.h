      SUBROUTINE ana_diag (ng, tile, model)
!
!! svn $Id$
!!======================================================================
!! Copyright (c) 2002-2007 The ROMS/TOMS Group                         !
!!   Licensed under a MIT/X style license                              !
!!   See License_ROMS.txt                                              !
!!                                                                     !
!=======================================================================
!                                                                      !
!  This routine is provided so the USER can compute any specialized    !
!  diagnostics.  If activated, this routine is call at end of every    !
!  3D-equations timestep.                                              !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_ncparam
      USE mod_ocean
!
! Imported variable declarations.
!
      integer, intent(in) :: ng, tile, model

#include "tile.h"
!
      CALL ana_diag_tile (ng, model, Istr, Iend, Jstr, Jend,            &
     &                    LBi, UBi, LBj, UBj,                           &
     &                    OCEAN(ng) % ubar,                             &
     &                    OCEAN(ng) % vbar,                             &
     &                    OCEAN(ng) % u,                                &
     &                    OCEAN(ng) % v)
!
! Set analytical header file name used.
!
      IF (Lanafile) THEN
        ANANAME( 5)='ROMS/Functionals/ana_diag.h'
      END IF

      RETURN
      END SUBROUTINE ana_diag
!
!***********************************************************************
      SUBROUTINE ana_diag_tile (ng, model, Istr, Iend, Jstr, Jend,      &
     &                          LBi, UBi, LBj, UBj,                     &
     &                          ubar, vbar, u, v)
!***********************************************************************
!
      USE mod_param
      USE mod_iounits
      USE mod_scalars
#ifdef SEAMOUNT
      USE mod_stepping
#endif
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, model, Iend, Istr, Jend, Jstr
      integer, intent(in) :: LBi, UBi, LBj, UBj
!
#ifdef ASSUMED_SHAPE
      real(r8), intent(in) :: ubar(LBi:,LBj:,:)
      real(r8), intent(in) :: vbar(LBi:,LBj:,:)
      real(r8), intent(in) :: u(LBi:,LBj:,:,:)
      real(r8), intent(in) :: v(LBi:,LBj:,:,:)
#else
      real(r8), intent(in) :: ubar(LBi:UBi,LBj:UBj,3)
      real(r8), intent(in) :: vbar(LBi:UBi,LBj:UBj,3)
      real(r8), intent(in) :: u(LBi:UBi,LBj:UBj,N(ng),2)
      real(r8), intent(in) :: v(LBi:UBi,LBj:UBj,N(ng),2)
#endif
!
!  Local variable declarations.
!
      integer :: i, j, k
      real(r8) :: umax, ubarmax, vmax, vbarmax

#ifdef SEAMOUNT
!
!  Open USER file.
!
      IF (iic(ng).eq.ntstart(ng)) THEN
        OPEN (usrout,file=USRname,form='formatted',status='unknown',    &
     &        err=40)
        GO TO 60
  40    WRITE (stdout,50) USRname
  50    FORMAT (' ANA_DIAG - unable to open output file: ',a)
        exit_flag=2
  60    CONTINUE
      END IF
!
!  Write out maximum values of velocity.
!
      umax=0.0_r8
      vmax=0.0_r8
      ubarmax=0.0_r8
      vbarmax=0.0_r8
      DO k=1,N(ng)
        DO j=0,Mm(ng)+1
          DO i=1,Lm(ng)+1
            umax=MAX(umax,u(i,j,k,nnew(ng)))
          END DO
        END DO
        DO j=1,Mm(ng)+1
          DO i=0,Lm(ng)+1
            vmax=MAX(vmax,v(i,j,k,nnew(ng)))
          END DO
        END DO
      END DO
      DO j=0,Mm(ng)+1
        DO i=1,Lm(ng)+1
          ubarmax=MAX(ubarmax,ubar(i,j,knew(ng)))
        END DO
      END DO
      DO j=1,Mm(ng)+1
        DO i=0,Lm(ng)+1
          vbarmax=MAX(vbarmax,vbar(i,j,knew(ng)))
        END DO
      END DO
!
!  Write out maximum values on velocity.
!
      WRITE (usrout,70) tdays(ng), ubarmax, vbarmax, umax, vmax
  70  FORMAT (2x,f13.6,2x,1pe13.6,2x,1pe13.6,2x,1pe13.6,2x,1pe13.6)
#endif
      RETURN
      END SUBROUTINE ana_diag_tile