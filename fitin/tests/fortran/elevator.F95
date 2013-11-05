#define POWER 4
#define OPERATING 2
#define OVERLOAD 1

integer function get_weight()
  implicit none
  get_weight = 150
end function

program elevator
  implicit none
  integer :: state = IOR(POWER, OPERATING), weight, get_weight

  do while(IAND(state, POWER) == POWER)
    weight = get_weight()

    if(IAND(state, OPERATING) == OPERATING) then
      if(weight < 0) then
        state = 0
      else if(weight <= 750) then
        state = IAND(state, NOT(OVERLOAD))
      else
        state = IOR(state, OVERLOAD)
      end if

      if(IAND(state, OVERLOAD) == OVERLOAD) then
        write(*,*) 'Elevator overloaded.'
      else if(state == 0) then
        write(*,*) 'Elevator shutting down!'
      else
        write(*,*) 'System operating.'
      end if
    end if

    call sleep(1)
  end do
end program

