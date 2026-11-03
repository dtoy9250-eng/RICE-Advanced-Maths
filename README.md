# RICE Advanced Maths — Casio fx-CG50

This is a private project for an educational maths companion add-in for the Casio fx-CG50.

## Project source

The complete source project is in [RICE_Source_Clean.tar.gz](./RICE_Source_Clean.tar.gz). It includes the C source, calculator icons, host-side tests, CMake configuration and project documentation.

## Build on GitHub (no setup on your Windows laptop)

1. Open the **Actions** tab.
2. Select **Build RICE for fx-CG50**.
3. Choose **Run workflow** and confirm.
4. Open the resulting run and wait for the build to finish.
5. If it succeeds, download the **RICE-fx-CG50** artifact and extract `RICE.g3a`.

The build runs on a GitHub-hosted Linux runner. You do not need to install Linux, CMake, the fxSDK or a compiler on your own Windows computer.

## Install on the calculator

1. Connect the fx-CG50 to the computer by USB.
2. Select **USB Flash** on the calculator.
3. Open its storage in File Explorer.
4. Copy `RICE.g3a` to the calculator's `@ADDIN` folder.
5. Safely eject the calculator.

Only use the `.g3a` produced by a successful build. A successful cloud compile is not the same as a physical hardware test, so check the add-in starts and its menus respond correctly on your calculator.

## Tests and scope

The source archive includes host-side regression tests. These are useful for checking the maths engine and simulated interface, but they do not replace the SuperH target build or a test on a real fx-CG50.

RICE is a visible educational learning tool. It is not designed to hide features or bypass examination rules.
