#%%

import gzip
import json
from pandas.io.json import json_normalize
import numpy as np


class CustomSerializer(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, np.int64):
            return int(o)
        if isinstance(o, np.uint64):
            return int(o)
        return super().default(o)


#%%

with gzip.open("./tmp/gas-measurements.jsonl.gz") as f:
    df = json_normalize([json.loads(line) for line in f])

df["transaction.gas_used"] = df["transaction.gas_used"].astype(np.float64)
df["gas_per_second"] =  df["transaction.gas_used"] / df["usage.clock_time"]
df.sort_values(by="gas_per_second", inplace=True)

#%%


# with open("", "w") as f:
#     json.dump(df.iloc[3].to_dict(), f, cls=CustomSerializer)

df.iloc[10].to_dict()
