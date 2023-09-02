## ProcessRunner

ProcessRunner is a simple console app that launches a process, waits for it to exit, then launches it again in a loop. This provides a basic mechanism for automatically relaunching a program when it crashes.

## Usage

The simplest way to use this is to create a one-line batch script next to the process you want to run that looks something like the following. Replace anything in square brackets \[\] with your own information.

```
start "ProcessRunner: [MyAppName]" [Path to ProcessRunner]ProcessRunner.exe "[MyApp].exe [command line params for my app] %*"
```

Note that if your custom command line params contain any quotations ("), you will need to double them ("").

## Releases

Release can be found [here](https://github.com/CrystalFerrai/ProcessRunner/releases).

## Building

To build from source, just open the SLN file in Visual Studio 2022 and build it.

## Platforms Supported

**Windows Only**

ProcessRunner calls directly into Windows APIs. Other platforms have similar APIs, but only Windows has been implemented. Therefore, this library will not work on other platforms.