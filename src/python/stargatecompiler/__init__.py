import os
import sys
import subprocess
from pathlib import Path

__version__ = "0.1.0"

def get_executable_path():
    """Get the path to the compiled C++ executable."""
    import site
    import importlib.util

    # Define possible executable names
    executable_name = "stargate"  # Adjust this to match your actual binary name

    # Search locations in order of preference
    search_locations = []

    # 1. Try to find the actual installed package location
    try:
        # Get the installed package location
        spec = importlib.util.find_spec("stargatecompiler")  # Your actual module name
        if spec and spec.origin:
            installed_module_dir = Path(spec.origin).parent
            search_locations.append(installed_module_dir)
    except:
        pass

    # 2. Current module directory (editable install source)
    module_dir = Path(__file__).parent
    search_locations.append(module_dir)

    # 3. Site-packages locations
    for site_path in site.getsitepackages():
        search_locations.append(Path(site_path) / "stargatecompiler")
        search_locations.append(Path(site_path) / "stargate")

    # 4. User site-packages
    try:
        user_site = site.getusersitepackages()
        if user_site:
            search_locations.append(Path(user_site) / "stargatecompiler")
            search_locations.append(Path(user_site) / "stargate")
    except:
        pass

    # 5. Virtual environment site-packages (if we're in a venv)
    if hasattr(sys, 'prefix') and sys.prefix != sys.base_prefix:
        # We're in a virtual environment
        venv_site = Path(sys.prefix) / "lib" / f"python{sys.version_info.major}.{sys.version_info.minor}" / "site-packages"
        search_locations.append(venv_site / "stargatecompiler")
        search_locations.append(venv_site / "stargate")

    # Search all locations
    for location in search_locations:
        if not location.exists():
            continue

        executable_path = location / executable_name
        if executable_path.exists() and os.access(executable_path, os.X_OK):
            return str(executable_path)

    print(f"ERROR: Executable '{executable_name}' not found in any location:", file=sys.stderr)
    raise FileNotFoundError(f"Executable '{executable_name}' not found in any expected location")

def main():
    """Entry point for the console script."""
    try:
        executable_path = get_executable_path()

        # Execute the C++ binary directly, replacing the current process
        os.execv(executable_path, [executable_path] + sys.argv[1:])

    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}", file=sys.stderr)
        # Fallback to subprocess if execv fails
        try:
            executable_path = get_executable_path()
            result = subprocess.run([executable_path] + sys.argv[1:])
            sys.exit(result.returncode)
        except:
            sys.exit(1)

