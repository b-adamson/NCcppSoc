import os

# Set the environment variables
PYTHONHOME = "C:/msys64/mingw64"
PYTHONPATH = "C:/Users/beada/dev/Projects/festi/billy-adamson/festi/.venv/lib/python3.12/site-packages"

# Set PYTHONHOME environment variable
if not os.environ.get('PYTHONHOME'):
    os.environ['PYTHONHOME'] = PYTHONHOME
else:
    raise RuntimeError("Failed to set PYTHONHOME environment variable")

# # Set PYTHONPATH environment variable
# if not os.environ.get('PYTHONPATH'):
#     os.environ['PYTHONPATH'] = PYTHONPATH
# else:
#     raise RuntimeError("Failed to set PYTHONPATH environment variable")

# try:
#     os.add_dll_directory(PYTHONHOME + r'\bin')
# except:
#     print("Failed to add PYTHONHOME/bin/ directory to dll search path. Importing some modules may return issues.")

