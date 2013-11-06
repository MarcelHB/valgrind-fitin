module FITIN
  interface
    subroutine FITIN_MONITOR_MEMORY(ptr, size)
      implicit none
      integer(__SIZEOF_POINTER__) :: ptr, size
    end subroutine
  end interface

  contains

  subroutine FITIN_MONITOR_VARIABLE(var)
    implicit none
    integer :: var

    call FITIN_MONITOR_MEMORY(LOC(var), SIZEOF(var))
  end subroutine
end module
