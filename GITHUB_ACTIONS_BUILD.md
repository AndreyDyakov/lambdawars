# Lambda Wars - GitHub Actions Build Setup

This repository includes GitHub Actions workflow for building Lambda Wars on Windows (Visual Studio 2013) via GitHub Actions.

## Quick Start

### 1. Push to GitHub
```bash
cd /home/a/Загрузки/lambdawars
git init
git add .
git commit -m "Initial commit with GitHub Actions build"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/lambdawars.git
git push -u origin main
```

### 2. Enable GitHub Actions
1. Go to your repo on GitHub: `https://github.com/YOUR_USERNAME/lambdawars`
2. Click **Actions** tab
3. Click **I understand my workflows, go ahead and enable them**

### 3. Trigger Build
- **Automatic**: Any push to `main`/`master` triggers build
- **Manual**: Go to Actions → "Build Lambda Wars" → "Run workflow"

### 4. Get Artifacts
After build completes (5-15 minutes):
1. Go to Actions → Build Lambda Wars → latest run
2. Scroll down to **Artifacts**
3. Download `lambdawars-build-<sha>.zip`
3. Extract → DLLs are inside

## Workflow Details

| Setting | Value |
|---------|-------|
| Runner | `windows-latest` (GitHub hosted) |
| VS Version | Visual Studio 2013 (via `ilammy/msvc-dev-cmd@v1`) |
| Architecture | Win32 (32-bit) |
| Configuration | Release |
| Python | 2.7.18 (required for build scripts) |

## Build Time
- **First run**: ~10-15 min (installs VS2013, Python, deps)
- **Cached runs**: ~5-8 min

## Artifacts
Download contains:
```
build_artifacts/
├── server.dll          # Game server DLL
├── client.dll          # Game client DLL
├── ...other DLLs
```

## Local Testing (Linux)
After downloading artifacts:
```bash
# Copy to Steam game directory
cp *.dll ~/.steam/root/steamapps/common/Lambda\ Wars/lambdawars/bin/
# Launch via Steam with Proton GE
```

## Troubleshooting

### Build Fails
1. Check **Actions** tab → failed run → logs
2. Common issues:
   - Missing dependencies → check `hl2wars_shareddefs.h` paths
   - Python errors → ensure Python 2.7 is installed
   - Missing SDK → Source SDK 2013 must be in expected location

### No Artifacts
- Build succeeded but no DLLs found
- Check `build_artifacts` step logs
- May need to adjust output paths in `.vcxproj` files

## Customization

### Change Build Config
Edit `.github/workflows/build.yml`:
```yaml
env:
  CONFIGURATION: Release  # or Debug
  PLATFORM: Win32         # or x64 (if supported)
```

### Add Dependencies
```yaml
- name: Install additional deps
  run: |
    choco install directxsdk -y
    # or copy SDK files to expected location
```

### Build on Schedule
```yaml
on:
  schedule:
    - cron: '0 2 * * 0'  # Weekly Sunday 2AM
```

## Security Notes
- **No secrets needed** for basic build
- If adding Steam API keys or signing certificates → use GitHub Secrets
- Artifacts are public if repo is public

## Cost
- **Free** for public repos (unlimited minutes)
- Private repos: 2000 min/month free tier

## Next Steps
1. Push this repo to GitHub
2. Enable Actions
3. Watch first build
3. Download DLLs → test on Linux via Proton
4. Iterate on Python AI changes → push → auto-build → test