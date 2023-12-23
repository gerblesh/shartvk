
_default:
        @just --unstable --list --list-heading $'Available commands:\n' --list-prefix $' - '

setup-fedora:
        sudo dnf install $(cat packages-fedora)

clean:
        rm -rf target

build:
        if [ ! -d "target" ]; then \
                mkdir target; \
        fi

        cd target && \
        meson setup ..

        cd target && \
        ninja

run: build
        VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./target/shartvk
