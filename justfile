
_default:
        @just --unstable --list --list-heading $'Available commands:\n' --list-prefix $' - '

setup-fedora:
        sudo dnf install $(cat packages-fedora)



setup-cglm:
        cd cglm && \
        sh autogen.sh
        cd cglm && \
        sh configure

cglm:
        cd cglm && \
        make
        cp -r cglm/include/cglm src/include/

check-cglm:
        cd cglm && \
        make check

build: cglm
        if [ ! -d "target" ]; then \
                mkdir target; \
        fi

        #gcc -o target/shartvk src/main.c -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -Wall -g
        
        #gcc -c -o target/main.o src/main.c -Icglm/include -Isrc/include # Compile main.c

        # Linking step: specify the path to cglm library and link statically
        gcc -o target/shartvk src/main.c -Lcglm/.libs -Wl,-Bstatic -lcglm -Wl,-Bdynamic -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lwayland-client -lwayland-cursor -lwayland-egl


run: build
        VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./target/shartvk
