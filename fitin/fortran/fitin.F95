module FITIN
  interface
    subroutine FITIN_MONITOR_MEMORY(ptr, size)
      implicit none
      integer(__SIZEOF_POINTER__) :: ptr, size
    end subroutine

    subroutine FITIN_MONITOR_FIELD_F(ptr, size, dims, dims_size)
      implicit none
      integer(__SIZEOF_POINTER__) :: ptr, size, dims_size
      integer(__SIZEOF_POINTER__), dimension(:) :: dims
    end

    subroutine FITIN_UNMONITOR_MEMORY(ptr, size)
      implicit none
      integer(__SIZEOF_POINTER__) :: ptr, size
    end subroutine
  end interface
end module
