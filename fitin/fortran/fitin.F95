module FITIN
  interface
    subroutine FITIN_MONITOR_MEMORY(ptr, size)
      implicit none
      integer(__SIZEOF_POINTER__) :: ptr, size
    end subroutine

    subroutine FITIN_UNMONITOR_MEMORY(ptr, size)
      implicit none
      integer(__SIZEOF_POINTER__) :: ptr, size
    end subroutine
  end interface
end module
