#include "runtime.hpp"

extern "C" void relink_cpu_exec();

int main(int argc, char** argv) {
    rt::init(argc, argv);
    relink_cpu_exec();
    return 0;
}
