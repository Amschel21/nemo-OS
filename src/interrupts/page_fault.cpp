#include "../kernel/panic.hpp"

extern "C" void page_fault_handler()
{
    PANIC("PAGE FAULT");
}