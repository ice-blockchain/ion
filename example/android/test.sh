#!/bin/sh
cp -r src/* ./test/ion/src/main/java/
mkdir -p ./test/ion/src/cpp/prebuilt/
cp -r libs/* ./test/ion/src/cpp/prebuilt/
cd test
./gradlew connectedAndroidTest
