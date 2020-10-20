#include "core.h"

int main(int argc, char** argv)
{
    if (int ret = core::initialize()) return ret;
    if (int ret = core::init_app_and_start()) return ret;

    return 0;
}