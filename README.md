# DDCStreamer

MIL-STD-1553 monitor + JSON over UDP streamer skeleton for DDC aceXtreme hardware.

## Features
- Opens DDC (aceXtreme) device (placeholder logic, integrate with SDK).
- Monitors 1553 bus messages.
- Parses basic RT/SA/Word Count meta.
- Publishes JSON messages via UDP.
 - Configuration-driven field extraction (multi-word, bit level, scaling, direction R/T).
 - Optional batch mode at fixed output rate (e.g., 50 Hz) or per-value immediate streaming.
 - Optional CSV logging.
 - Simulation modu: Donanım olmadan RT/SA setlerine göre rastgele 1553 word üretimi.
 - Veri tipleri: raw, float (ölçekli int), signed (16/32+/N-bit birleşik), ieee754 (32-bit).
 - Sequence number (seq) alanı her paket için.

## Build
### Standard (MinGW example)
```powershell
# Configure (adjust -DDC_SDK_ROOT if/when real SDK integrated)
cmake -S . -B build -G "MinGW Makefiles" -DDC_SDK_ROOT="C:/DDC/aceXtremeSDK"
cmake --build build -j 4
```

### Portable static bundle (one-line)
```powershell
./build_portable.ps1    # Debug portable build + bundle
./build_portable.ps1 -Release   # Release portable build
./build_portable.ps1 -Clean -Release  # Clean rebuild
```

### CSV otomatik adlandırma
Config içinde `"csv_path": "auto"` kullanılırsa dosya adı `log_YYYYMMDD_HHMMSS.csv` formatında oluşturulur.
Token desteği: `{Y}{m}{d}{H}{M}{S}` veya `{datetime}` (örn: `logs/run_{datetime}.csv`).
Output executable + configs: `build_portable/portable/`.

### Notes
- `SIMULATION_ONLY` and `PORTABLE_BUILD` are enabled in the portable script.
- For MSVC static CRT: pass `-DPORTABLE_BUILD=ON -G "Visual Studio 17 2022"` and build Release.

## Run
```powershell
# Explicit config
./build_portable/portable/DDCStreamerApp.exe -c config.nested.sample.json

# Auto-select (no -c): order -> config.nested.sample.json, config.sample.json, config.json
./build_portable/portable/DDCStreamerApp.exe

# Override UDP port
./build_portable/portable/DDCStreamerApp.exe -p 9999
```

## JSON Message Example
Immediate mode (per value): {"name":"velocity","value":123.45,"ts":123456789}
Batch mode (snapshot): {"velocity":123.45,"txaInit":1,"altitude":1024.0,"timestamp_us":1234567,"timestamp":1.234567}

Nested alan desteği: Konfig dosyasında `production.line_1.speed` şeklindeki isimler batch JSON'da iç içe obje oluşturur.

Timestamp unit: microseconds since application start (replace with hardware time tag if available).

Scaling: use `lsb_exp` (power-of-two). Example: `lsb_exp: -7` => scale = 2^-7.

Simulation:
	"simulation": true,
	"sim_rate_hz": 50.0
	"sim_pattern": "random"  // veya "increment"

## Next Integration Steps
1. Integrate real aceXtreme monitor API (replace simulation).
2. Confirm endianness & combination order for multi-word numeric fields.
3. Add signed interpretation & IEEE754 float decode if required (current float assumes scaled int -> engineering units).
4. Add error/status flags and include in extraction config (parity, sync, gap time, retries).
5. Implement filtering pre-parse for performance (RT/SA masks).
6. Optimize UDP batching and add sequence numbers.
7. Add watchdog & stats (dropped messages, extraction errors).

## License
Internal / Proprietary.
