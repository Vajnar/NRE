#include <kobj/GlobalThread.h>
#include <stream/Serial.h>
#include <services/Console.h>
#include <stream/VGAStream.h>
#include <util/Atomic.h>
#include <CPU.h>
#include <kobj/Sm.h>
#include <kobj/Pt.h>

using namespace nre;

typedef struct {
  capsel_t sel;
  int value;
} sf;

sf sm1, sm2;

auto &ser = Serial::get();

static void dummy(void*) {
  sm1.sel = CapSelSpace::get().allocate();
  Syscalls::create_sm(sm1.sel, 0, Pd::current()->sel());
  
  while(1) {
    if(Atomic::add(&sm1.value, -1) <= 0)
        Syscalls::sm_ctrl(sm1.sel, Syscalls::SM_DOWN);
    
    Serial::get() << "ping\n";
    
    if(Atomic::add(&sm2.value, 1) < 0)
        Syscalls::sm_ctrl(sm2.sel, Syscalls::SM_UP);
  }
}

static void global_hello(void*) {
  sm2.sel = CapSelSpace::get().allocate();
  Syscalls::create_sm(sm2.sel, 1, Pd::current()->sel());
  sm2.value = 1;
  
  while(1) {
    if(Atomic::add(&sm2.value, -1) <= 0)
        Syscalls::sm_ctrl(sm2.sel, Syscalls::SM_DOWN);
    
    Serial::get() << "pong\n";
    
    if(Atomic::add(&sm1.value, 1) < 0)
        Syscalls::sm_ctrl(sm1.sel, Syscalls::SM_UP);
  }
}

int main(void) {
//  ConsoleSession cons("console", 1, "DiskTest");
//  s = new VGAStream(cons, 0);
//  int i;

//  s->clear(0);
//  *s << "Nazdar, svete\n";
//  *s >> i;
//  *s << "Cislo: " << i << "\n";
  
  GlobalThread::create(dummy, CPU::current().log_id(), "ctrl_world")->start();
  GlobalThread::create(global_hello, CPU::current().log_id(), "hello_world")->start();

//   Reference<LocalThread> ec = LocalThread::create(CPU::current().log_id());
//   Pt pt(ec, dummy);
  
  Sm sm(0);
  sm.down();
  return 0;
}
