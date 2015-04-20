#include "dialog.h"
#include "main.h"
#include <QApplication>
#include "screenxy.h"

/*
 * /sdaone/BOARDS/android/android-ndk-r19c
 * /sdaone/BOARDS/android/android-ndk-r19c/toolchains/llvm
 * /sdaone/BOARDS/android/android-ndk-r19c/sysroot/
 *
*/

bool __alive = true;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
    w.show();
    return a.exec();
}
