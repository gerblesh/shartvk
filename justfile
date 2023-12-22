_default:
        @just --unstable --list --list-heading $'Available commands:\n' --list-prefix $' - '


build:
        gcc src/main.c -o target/redshit-vulkan.x86_64
