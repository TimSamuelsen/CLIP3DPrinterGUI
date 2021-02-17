/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef PATTERNELEMENT_H
#define PATTERNELEMENT_H

#include <QListWidget>
struct PatternElement
{
    enum Color
    {
        RED = 1,
        GREEN = 2,
        BLUE = 4,
        YELLOW = RED + GREEN,
        CYAN = GREEN + BLUE,
        MAGENTA = RED + BLUE,
        WHITE = RED + GREEN + BLUE,
        NUM_COLORS = 8
    };

    QString name;
    int exposure;
    int darkPeriod;
    Color color;
    int bits;
    int splashImageIndex;
    int splashImageBitPos;
    bool trigIn;
    bool trigOut2;
    bool clear;
    bool selected;

    QColor getColor()
    {
        return QColor((color&1)*255, (color>>1&1)*255, (color>>2&1)*255);
    }

    PatternElement()
    {
        name = "";
        exposure = 0;
        color = RED;
        bits = 1;
        trigIn = true;
        selected = false;
        clear = true;
		splashImageIndex = 0;
    }
};

#endif //PATTERNELEMENT_H
