#!/usr/bin/env bash

set -Eeuo pipefail

project_dir=$(CDPATH= cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$project_dir"

build_dir="$project_dir/build"
output_dir="$project_dir/output"

required_packages=(
    build-essential
    cmake
    debhelper
    libglib2.0-dev
    libgtk-3-dev
    libibus-1.0-dev
    pkg-config
    ibus
)

require_linux() {
    if [[ "$(uname -s)" != "Linux" ]]; then
        printf '%s\n' 'This packaging flow is for Linux only.' >&2
        exit 1
    fi
}

run_as_root() {
    if [[ "$(id -u)" -eq 0 ]]; then
        "$@"
    else
        sudo "$@"
    fi
}

install_dependencies() {
    require_linux

    if ! command -v dpkg-query >/dev/null 2>&1; then
        printf '%s\n' 'dpkg-query is required to build Debian packages on this system.' >&2
        exit 1
    fi

    missing_packages=()
    for package in "${required_packages[@]}"; do
        if ! dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q 'install ok installed'; then
            missing_packages+=("$package")
        fi
    done

    if [[ ${#missing_packages[@]} -gt 0 ]]; then
        if [[ "$(id -u)" -ne 0 ]] && ! command -v sudo >/dev/null 2>&1; then
            printf '%s\n' 'sudo is required to install missing build dependencies.' >&2
            exit 1
        fi

        printf '\n%s\n' 'Installing missing packages:'
        printf '%s\n' "${missing_packages[@]}"

        run_as_root apt-get update
        run_as_root apt-get install -y "${missing_packages[@]}"
    fi
}

configure_project() {
    printf '\n========================================\n'
    printf '%s\n' 'Configuring Nudi'
    printf '%s\n' '========================================'

    cmake -S "$project_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
}

build_project() {
    configure_project

    printf '\n========================================\n'
    printf '%s\n' 'Building Nudi'
    printf '%s\n' '========================================'

    cmake --build "$build_dir" --parallel

    printf '\n========================================\n'
    printf '%s\n' 'Build completed successfully'
    printf '%s\n' '========================================'
}

test_project() {
    configure_project

    printf '\n========================================\n'
    printf '%s\n' 'Building and running tests'
    printf '%s\n' '========================================'

    cmake --build "$build_dir" --parallel

    run_tests
}

run_tests() {
    printf '\n========================================\n'
    printf '%s\n' 'Running CTest'
    printf '%s\n' '========================================'

    ctest --test-dir "$build_dir" --output-on-failure --verbose

    printf '\n========================================\n'
    printf '%s\n' 'All tests completed successfully'
    printf '%s\n' '========================================'
}

create_package() {
    printf '\n========================================\n'
    printf '%s\n' 'Creating Debian package'
    printf '%s\n' '========================================'

    dpkg-buildpackage -us -uc -b

    version_no=$(dpkg-parsechangelog -S Version)

    mkdir -p "$output_dir"

    find "$(dirname "$project_dir")" \
        -maxdepth 1 \
        -type f \
        \( \
            -name 'kannada-nudi*.deb' -o \
            -name 'kannada-nudi*.ddeb' -o \
            -name 'kannada-nudi*.buildinfo' -o \
            -name 'kannada-nudi*.changes' \
        \) \
        -exec mv -t "$output_dir" {} +

    package_path="$output_dir/kannada-nudi_${version_no}_amd64.deb"

    if [[ ! -f "$package_path" ]]; then
        printf '%s\n' 'Package was not found:' >&2
        printf '%s\n' "$package_path" >&2
        exit 1
    fi

    printf '\n%s\n' 'Package written to:'
    printf '%s\n' "$package_path"
}

install_package() {
    package_version=$(dpkg-parsechangelog -S Version)
    package_path="$output_dir/kannada-nudi_${package_version}_amd64.deb"

    if [[ ! -f "$package_path" ]]; then
        printf '%s\n' 'Could not find the Nudi Debian package.' >&2
        printf '%s\n' "$package_path" >&2
        exit 1
    fi

    if dpkg-query -W -f='${db:Status-Status}' kannada-nudi 2>/dev/null | grep -qx 'installed'; then
        installed_version=$(dpkg-query -W -f='${Version}' kannada-nudi)
        if dpkg --compare-versions "$installed_version" ge "$package_version"; then
            return 0
        fi
    fi

    run_as_root env DEBIAN_FRONTEND=noninteractive \
        apt-get install -y -qq "$package_path"
}

uninstall_package() {
    printf '\n========================================\n'
    printf '%s\n' 'Uninstalling Nudi'
    printf '%s\n' '========================================'

    if dpkg-query -W -f='${db:Status-Status}' kannada-nudi 2>/dev/null | grep -qx 'installed'; then
        run_as_root apt purge -y kannada-nudi
    else
        printf '%s\n' 'kannada-nudi is not installed; nothing to uninstall.'
    fi

    if command -v ibus >/dev/null 2>&1; then
        ibus exit >/dev/null 2>&1 || true
    fi
}

clean_project() {
    printf '\n========================================\n'
    printf '%s\n' 'Cleaning generated files'
    printf '%s\n' '========================================'

    rm -rf -- "$build_dir" "$output_dir"
    printf '%s\n' 'Build and package output directories removed.'
}

reinstall_and_run() {
    if [[ "$(id -u)" -eq 0 ]]; then
        printf '%s\n' 'Please run this script as your normal desktop user, not root.' >&2
        exit 1
    fi

    uninstall_package
    clean_project
    build_and_run
}

restart_ibus() {
    printf '\n========================================\n'
    printf '%s\n' 'Restarting IBus'
    printf '%s\n' '========================================'

    ibus-daemon --panel disable --xim --daemonize --replace || true

    local i=0
    while [[ "$i" -lt 20 ]]; do
        if ibus list-engine >/dev/null 2>&1; then
            break
        fi
        i=$((i + 1))
        sleep 1
    done

    ibus list-engine >/dev/null 2>&1 || {
        printf '%s\n' 'IBus did not become available after restart.' >&2
        return 1
    }
}

wait_for_nudi() {
    local i=0

    while [[ "$i" -lt 20 ]]; do
        if ibus list-engine 2>/dev/null | grep -q '[[:space:]]nudi[[:space:]]'; then
            printf '%s\n' 'Nudi engine registered successfully.'
            return 0
        fi

        i=$((i + 1))
        sleep 1
    done

    printf '%s\n' 'IBus did not register Nudi in this session.' >&2
    exit 1
}

activate_nudi() {
    printf '\n========================================\n'
    printf '%s\n' 'Activating Nudi'
    printf '%s\n' '========================================'

    wait_for_nudi

    ibus engine nudi

    local j=0
    while [[ "$j" -lt 10 ]]; do
        current_engine=$(ibus engine 2>/dev/null | tr -d '[:space:]' || true)

        if [[ "$current_engine" == "nudi" ]]; then
            printf '%s\n' 'Nudi activated successfully.'
            return 0
        fi

        j=$((j + 1))
        sleep 1
    done

    printf '%s\n' 'Nudi was registered but could not be activated.' >&2
    exit 1
}

build_and_run() {
    if [[ "$(id -u)" -eq 0 ]]; then
        printf '%s\n' 'Please run this script as your normal desktop user, not root.' >&2
        exit 1
    fi

    install_dependencies
    build_project

    printf '\n========================================\n'
    printf '%s\n' 'Running tests before packaging'
    printf '%s\n' '========================================'

    run_tests
    create_package
    install_package
    restart_ibus
    activate_nudi

    printf '\n========================================\n'
    printf '%s\n' 'SUCCESS'
    printf '%s\n' 'Nudi was built, tested, installed and activated.'
    printf '%s\n' '========================================'
}

show_menu() {
    printf '\n'
    printf '========================================\n'
    printf '        Kannada Nudi Development\n'
    printf '========================================\n'
    printf '\n'
    printf '1) Build & Package\n'
    printf '2) Build & Run\n'
    printf '3) Test\n'
    printf '4) Uninstall\n'
    printf '5) Clean\n'
    printf '6) Uninstall, clean, rebuild & run\n'
    printf '7) Exit\n'
    printf '\n'
    printf 'Select an option: '
}

case "${1:-}" in
    "")
        show_menu
        read -r choice
        case "$choice" in
            1)
                install_dependencies
                build_project
                create_package
                ;;
            2)
                build_and_run
                ;;
            3)
                install_dependencies
                test_project
                ;;
            4)
                uninstall_package
                ;;
            5)
                clean_project
                ;;
            6)
                reinstall_and_run
                ;;
            7)
                printf '%s\n' 'Exiting.'
                exit 0
                ;;
            *)
                printf '%s\n' 'Invalid option.'
                exit 1
                ;;
        esac
        ;;

    build)
        install_dependencies
        build_project
        create_package
        ;;

    run)
        build_and_run
        ;;

    test)
        install_dependencies
        test_project
        ;;

    uninstall)
        uninstall_package
        ;;

    clean)
        clean_project
        ;;

    reinstall)
        reinstall_and_run
        ;;

    package)
        install_dependencies
        build_project
        create_package
        ;;

    --help|-h)
        printf '%s\n' "Usage: $0 [build|run|test|package|uninstall|clean|reinstall]"
        printf '\n'
        printf '%s\n' 'Without arguments, an interactive menu is displayed.'
        exit 0
        ;;

    *)
        printf '%s\n' "Unknown command: $1" >&2
        printf '%s\n' "Usage: $0 [build|run|test|package|uninstall|clean|reinstall]" >&2
        exit 1
        ;;
 esac
