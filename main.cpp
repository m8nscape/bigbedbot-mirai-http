#include "core.h"

int main(int argc, char** argv)
{
    if (!core::initialize())
    {
        core::startup();
    }
    return 0;
}