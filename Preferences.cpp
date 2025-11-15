#include "Preferences.h"

#include <Json5.h>

bool Preferences::read(const QString& filename) {
    Json5Document doc;
    if (doc.read(filename)) {
        // read and validate all of the things
        return true;
    }
    return false;
}
