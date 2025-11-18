#!/bin/bash

# Cryptopix-CLWE Release Script
# Automates the release process for the universal C++ library

set -e

# Configuration
PROJECT_NAME="cryptopix-clwe"
VERSION_FILE="CMakeLists.txt"
CHANGELOG_FILE="CHANGELOG.md"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Get current version from CMakeLists.txt
get_current_version() {
    grep "project(clwe_avx VERSION" CMakeLists.txt | sed 's/.*VERSION \([0-9.]*\).*/\1/'
}

# Update version in CMakeLists.txt
update_version() {
    local new_version="$1"
    log_info "Updating version to $new_version in CMakeLists.txt"

    # Update project version
    sed -i.bak "s/project(clwe_avx VERSION [0-9.]*/project(clwe_avx VERSION $new_version/" CMakeLists.txt

    # Update conanfile.py
    sed -i.bak "s/version = \"[0-9.]*\"/version = \"$new_version\"/" conanfile.py

    # Update vcpkg port
    sed -i.bak "s/\"version\": \"[0-9.]*\"/\"version\": \"$new_version\"/" vcpkg/ports/clwe/vcpkg.json

    log_success "Version updated to $new_version"
}

# Update changelog
update_changelog() {
    local new_version="$1"
    local date=$(date +%Y-%m-%d)

    log_info "Updating changelog for version $new_version"

    # Create new version entry
    local header="## [$new_version] - $date"
    local changelog_content=$(cat << EOF

### Added
- [List new features here]

### Changed
- [List changes here]

### Fixed
- [List bug fixes here]

EOF
)

    # Insert after the header
    sed -i.bak "/## \[Unreleased\]/a\\
$header\\
$changelog_content\\
" "$CHANGELOG_FILE"

    log_success "Changelog updated"
}

# Create git tag
create_git_tag() {
    local version="$1"
    local tag="v$version"

    log_info "Creating git tag $tag"

    if git tag -l | grep -q "^$tag$"; then
        log_warning "Tag $tag already exists"
        read -p "Overwrite existing tag? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            git tag -d "$tag"
            git push origin :refs/tags/"$tag" 2>/dev/null || true
        else
            log_info "Keeping existing tag"
            return
        fi
    fi

    git add .
    git commit -m "Release version $version" || log_warning "No changes to commit"
    git tag -a "$tag" -m "Release version $version"
    git push origin main
    git push origin "$tag"

    log_success "Git tag $tag created and pushed"
}

# Build and test release
build_release() {
    local version="$1"

    log_info "Building release version $version"

    # Clean build
    rm -rf build/
    mkdir build
    cd build

    # Configure and build
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_PYTHON_BINDINGS=OFF \
        -DENABLE_TESTS=OFF \
        -DENABLE_BENCHMARKS=ON \
        -DCMAKE_INSTALL_PREFIX=./install

    make -j$(nproc)
    make install

    # Run tests
    ./benchmark_color_kem_timing --quick
    ./demo_kem

    cd ..
    log_success "Release build completed"
}

# Create release archives
create_archives() {
    local version="$1"
    local archive_name="$PROJECT_NAME-$version"

    log_info "Creating release archives"

    # Source archive
    git archive --format=tar.gz -o "$archive_name-src.tar.gz" --prefix="$PROJECT_NAME-$version/" HEAD

    # Binary archive (if build exists)
    if [ -d "build/install" ]; then
        cd build/install
        tar -czf "../../$archive_name-linux-x64.tar.gz" .
        cd ../..
    fi

    log_success "Release archives created"
}

# Generate checksums
generate_checksums() {
    local version="$1"

    log_info "Generating checksums"

    for file in "$PROJECT_NAME-$version"*.tar.gz; do
        if [ -f "$file" ]; then
            sha256sum "$file" > "$file.sha256"
            log_info "Generated checksum for $file"
        fi
    done

    log_success "Checksums generated"
}

# Print usage
usage() {
    cat << EOF
Cryptopix-CLWE Release Script

USAGE:
    $0 [OPTIONS] <version>

ARGUMENTS:
    version         New version number (e.g., 1.0.1)

OPTIONS:
    --dry-run       Show what would be done without making changes
    --skip-build    Skip the build and test step
    --skip-tag      Skip git tagging
    --help, -h      Show this help

EXAMPLES:
    $0 1.0.1                    # Create release version 1.0.1
    $0 --dry-run 1.0.1          # Show what would be done
    $0 --skip-build 1.0.1       # Skip build step

EOF
}

# Main function
main() {
    local dry_run=false
    local skip_build=false
    local skip_tag=false
    local new_version=""

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --dry-run)
                dry_run=true
                shift
                ;;
            --skip-build)
                skip_build=true
                shift
                ;;
            --skip-tag)
                skip_tag=true
                shift
                ;;
            --help|-h)
                usage
                exit 0
                ;;
            -*)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
            *)
                if [ -z "$new_version" ]; then
                    new_version="$1"
                else
                    log_error "Multiple versions specified"
                    usage
                    exit 1
                fi
                shift
                ;;
        esac
    done

    if [ -z "$new_version" ]; then
        log_error "Version number is required"
        usage
        exit 1
    fi

    # Validate version format
    if ! echo "$new_version" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        log_error "Invalid version format. Use semantic versioning (e.g., 1.0.1)"
        exit 1
    fi

    local current_version=$(get_current_version)
    log_info "Current version: $current_version"
    log_info "New version: $new_version"

    if [ "$dry_run" = true ]; then
        log_info "DRY RUN - No changes will be made"
    fi

    # Confirm release
    echo
    log_warning "This will create a new release with the following changes:"
    echo "  - Update version from $current_version to $new_version"
    echo "  - Update changelog"
    echo "  - Create git tag v$new_version"
    echo "  - Build and test release"
    echo "  - Create release archives"
    echo
    read -p "Continue with release? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        log_info "Release cancelled"
        exit 0
    fi

    # Execute release steps
    if [ "$dry_run" = false ]; then
        update_version "$new_version"
        update_changelog "$new_version"
    fi

    if [ "$skip_build" = false ]; then
        build_release "$new_version"
    fi

    create_archives "$new_version"
    generate_checksums "$new_version"

    if [ "$skip_tag" = false ] && [ "$dry_run" = false ]; then
        create_git_tag "$new_version"
    fi

    echo
    log_success "Release $new_version completed!"
    echo
    log_info "Next steps:"
    echo "  1. Create a GitHub release with the generated archives"
    echo "  2. Update package manager registries (vcpkg, Conan)"
    echo "  3. Announce the release"
    echo
    log_info "Release files created:"
    ls -la "$PROJECT_NAME-$new_version"*.tar.gz* 2>/dev/null || true
}

# Run main function with all arguments
main "$@"