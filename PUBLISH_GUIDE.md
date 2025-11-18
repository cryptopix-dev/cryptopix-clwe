# Publishing Cryptopix-CLWE to GitHub

Follow these steps to publish the cryptopix-clwe library to GitHub.

## Prerequisites

- Git installed
- GitHub account with repository created at https://github.com/cryptopix-dev/cryptopix-clwe

## Step-by-Step Commands

1. Navigate to the project directory:

```bash
cd cryptopix-clwe
```

2. Initialize git repository:

```bash
git init
```

3. Add all files to staging:

```bash
git add .
```

4. Commit the initial version:

```bash
git commit -m "Initial commit of Cryptopix-CLWE v1.0.0"
```

5. Rename branch to main (if not already):

```bash
git branch -M main
```

6. Add the GitHub remote:

```bash
git remote add origin https://github.com/cryptopix-dev/cryptopix-clwe.git
```

7. Push to GitHub:

```bash
git push -u origin main
```

8. Create a version tag:

```bash
git tag v1.0.0
```

9. Push the tag:

```bash
git push origin v1.0.0
```

10. Create a release on GitHub:

    - Go to https://github.com/cryptopix-dev/cryptopix-clwe/releases
    - Click "Create a new release"
    - Tag: v1.0.0
    - Title: Cryptopix-CLWE v1.0.0
    - Description: Initial release of the color-integrated CLWE cryptosystem library.
    - Optionally, upload the tar.gz files as assets.
    - Click "Publish release"

That's it! The library is now published on GitHub with the first release.