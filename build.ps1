param(
    [switch] $SkipGuiSmoke
)

$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & g++ -std=c++17 -Wall -Wextra -Werror -Isrc `
      tests/key_state_test.cpp src/key_state.cpp -o tests/key_state_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'key_state_test compile failed' }
    & .\tests\key_state_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'key_state_test failed' }

    & g++ -std=c++17 -Wall -Wextra -Werror -Isrc `
      tests/keyboard_mapper_test.cpp src/key_state.cpp src/keyboard_mapper.cpp `
      -o tests/keyboard_mapper_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'keyboard_mapper_test compile failed' }
    & .\tests\keyboard_mapper_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'keyboard_mapper_test failed' }

    & g++ -std=c++17 -Wall -Wextra -Werror -Isrc `
      tests/autostart_test.cpp src/autostart.cpp `
      -o tests/autostart_test.exe -ladvapi32
    if ($LASTEXITCODE -ne 0) { throw 'autostart_test compile failed' }
    & .\tests\autostart_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'autostart_test failed' }

    & g++ -std=c++17 -Wall -Wextra -Werror -Isrc `
      tests/lifecycle_test.cpp src/key_state.cpp src/keyboard_mapper.cpp `
      -o tests/lifecycle_test.exe -luser32 -lshell32 -ladvapi32
    if ($LASTEXITCODE -ne 0) { throw 'lifecycle_test compile failed' }
    & .\tests\lifecycle_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'lifecycle_test failed' }

    & powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\generate-icon.ps1
    if ($LASTEXITCODE -ne 0) { throw 'Icon generation failed' }
    & windres -I assets -i assets/app.rc -O coff -o assets/app.res.o
    if ($LASTEXITCODE -ne 0) { throw 'Windows resource compile failed' }

    & g++ -std=c++17 -O2 -Wall -Wextra -Werror -static -mwindows `
      src/main.cpp src/key_state.cpp src/keyboard_mapper.cpp src/autostart.cpp `
      assets/app.res.o -o CapsAsShift.exe -luser32 -lshell32 -ladvapi32
    if ($LASTEXITCODE -ne 0) { throw 'GUI EXE build failed' }

    if ($SkipGuiSmoke) {
        'GUI smoke test skipped (interactive desktop required).'
    } else {
        & powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\app_smoke.ps1
        if ($LASTEXITCODE -ne 0) { throw 'app smoke test failed' }
    }
    'C++ tests passed; CapsAsShift.exe built.'
} finally {
    Pop-Location
}
