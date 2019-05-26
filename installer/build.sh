#!/bin/bash

mkdir -p ROOT/tmp/RDCFilterWheel_X2/
cp "../filterwheellist rdc.txt" ROOT/tmp/RDCFilterWheel_X2/
cp "../build/Release/libRDCFilterWheel.dylib" ROOT/tmp/RDCFilterWheel_X2/

if [ ! -z "$installer_signature" ]; then
# signed package using env variable installer_signature
pkgbuild --root ROOT --identifier org.rti-zone.RDCFilterWheel_X2 --sign "$installer_signature" --scripts Scripts --version 1.0 RDCFilterWheel_X2.pkg
pkgutil --check-signature ./RDCFilterWheel_X2.pkg
else
pkgbuild --root ROOT --identifier org.rti-zone.RDCFilterWheel_X2 --scripts Scripts --version 1.0 RDCFilterWheel_X2.pkg
fi

rm -rf ROOT
