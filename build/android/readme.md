# USEFUL ADB COMMANDS

## BUILD IN DOCKER

You may need to add exec `sdkmanager "ndk;23.2.8568313"` just before `./gradlew assembleRelease`, the version must match the one shown in `app/build.gradle`.

```
winpty docker exec -it nice_goldwasser bash -c "cd /uli78/build/android && ./gradlew assembleRelease" && adb install -r app/build/outputs/apk/arm8/release/app-arm8-release.apk && adb shell am start -n com.uli78.uli/com.uli78.uli.TIC
```

## INSTALL APP AND RUN
```
release: adb install -r app/build/outputs/apk/arm8/release/app-arm8-release.apk && adb shell am start -n com.uli78.uli/com.uli78.uli.TIC

debug: adb install -r app/build/outputs/apk/arm8/debug/app-arm8-debug.apk && adb shell am start -n com.uli78.uli/com.uli78.uli.TIC
```

## VIEW APP LOG
```
adb logcat --pid `adb shell pidof com.uli78.uli`
```

## STOP APP
```
adb shell am force-stop com.uli78.uli
```
