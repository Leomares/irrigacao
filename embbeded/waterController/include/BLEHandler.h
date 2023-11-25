#ifndef BLEHANDLER_H
#define BLEHANDLER_H

#include <NimBLEDevice.h>

extern NimBLEServer *pServer;

class BLEHandler
{
public:
    BLEHandler();
    static void setup();
};
#endif