import sys
from ge import *

print("[*] Instantiating GAUSS Engine")

ge = GAUSS()

# Initialize the engine
if not ge.initialize():
    print("[-] GAUSS Engine failed to initialize")
    sys.exit(1)

print("[*] GAUSS Engine initialized successfully")

# Assign a symbol to the GAUSS workspace
ge['x'] = GEMatrix(list(range(1, 21)), 5, 4)

# Modify a symbol in GAUSS
ge.executeString("x = sin(x) + 50") 

# Print the symbol
ge.executeString("print x")

# Get output
print('[1] Output from GAUSS:\n{}'.format(ge.getOutput()))

# Get symbol directly from the workspace
x = ge['x']

# Get data back as Python type
print(x.getData())

# Run a file
ge.executeFile('{}/examples/ols.e'.format(ge.getHome()))

# Get the output again
print('[2] Output from GAUSS:\n{}'.format(ge.getOutput()))

print("[*] GAUSS Engine has been shutdown")

