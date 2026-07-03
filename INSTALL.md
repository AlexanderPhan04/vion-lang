# Install Vion

This guide installs the Vion CLI so you can run Vion programs from any terminal:

```powershell
vion main.vion
```

## Windows User Install

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install-windows.ps1
```

The installer:

- builds Vion in `Release` mode
- copies `vion.exe` to `%LOCALAPPDATA%\Programs\Vion\bin`
- copies docs and examples to `%LOCALAPPDATA%\Programs\Vion`
- adds `%LOCALAPPDATA%\Programs\Vion\bin` to your user `PATH`

Open a new terminal after installation, then verify:

```powershell
vion version
```

Run a program:

```powershell
vion .\examples\hello.vion
```

Or create `main.vion`:

```vion
print "Hello from Vion"
```

Then run:

```powershell
vion main.vion
```

## Custom Install Directory

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install-windows.ps1 -InstallDir "C:\Tools\Vion"
```

## Install Without Editing PATH

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install-windows.ps1 -NoPath
```

You can still run the executable directly:

```powershell
%LOCALAPPDATA%\Programs\Vion\bin\vion.exe main.vion
```

## Uninstall

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\uninstall-windows.ps1
```

## CMake Install

You can also install using CMake directly:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
cmake --install build --config Release --prefix "%LOCALAPPDATA%\Programs\Vion"
```

This copies the executable, docs, and examples, but it does not modify `PATH`.

## Create a ZIP Package

```powershell
cmake --build build --config Release
cpack --config build\CPackConfig.cmake -C Release
```

The generated `.zip` can be uploaded to a GitHub Release.
