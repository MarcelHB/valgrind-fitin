// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
namespace sihft {

typedef void (*fault_detected_handler)();
void set_fault_detected(fault_detected_handler handler) throw();
void fault_detected() throw() __attribute__((noreturn));

}
