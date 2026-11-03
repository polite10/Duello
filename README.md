# Düello

STM32U5A9J-DK'nin yuvarlak ekranında çalışan, PC'den kontrol edilen animasyon uygulaması. [Device Studio](https://github.com/polite10/DeviceStudio) komut gönderir; karakter, hedef ve animasyonlar cihazdaki C/LVGL firmware'i tarafından çizilir. Bilgisayardan video/piksel akışı gerekmez.

## Kontroller

| Device Studio düğmesi | UART komutu | Davranış |
|---|---|---|
| Silahı çıkar | DRAW | Silahı belden çıkarır, kolu 600 ms'de uzatır |
| Ateş et | FIRE | 250 ms ateş animasyonu; mermi ve hedef durumu güncellenir |
| Silahı yerine koy | HOLSTER | Silahı 600 ms'de bele yerleştirir |
| Yeniden doldur | RELOAD | 900 ms sonunda mermiyi 6 yapar |
| Yürü / Dur | WALK / STOP | Yürüme hareketi / bekleme |
| Sahneyi sıfırla | RESET | Başlangıç karakter, hedef ve mermi durumu |

Silah çıkarılmadan ateş edilmez. Animasyon devam ederken yeni eylem `busy` döndürür. Hedef üç isabetten sonra tamamlanır; RESET yeni sahne başlatır. Yürüme, beş saniye komut/heartbeat gelmezse durur. Device Studio durum sorgularını 1,2 saniyede bir yapar.

Uygulama saatin RTC/Flash kayıtlarına yazmaz. Saat uygulamasına dönmek için Smartwatch `.dfw` paketini yükleyin.

## Derleme ve yükleme

Yanına güncel [smart_watch](https://github.com/polite10/smart_watch) çalışma ağacı gerekir. `smart_watch-main`, `Smartwatch` ve `smart_watch` klasör adları otomatik aranır; farklı konum `--smartwatch-root` / `-SmartwatchRoot` ile verilir. SDK'nın güncel iki parametreli `device_runtime_init(app, handler)` DMA API'si ve `tools/device_build.py` yardımcısı kullanılır. `sdk.lock.json` temel commit/API bilgisidir; derleme yerel çalışma ağacını kullanır. ST'nin sürücüleri, mevcut kart ekran adaptörü, LVGL ve NemaGFX o repodan kullanılır; saat ekranları/modelleri Düello'ya bağlanmaz. Ekran/GPU adaptörleri yanında ortak UART yolunun `watch_led_commands` ayrıştırıcısı bağlanır.

macOS'ta:

```sh
python3 build.py
# SDK nesneleri güncelse:
python3 build.py --skip-sdk-build
```

Windows'ta Python 3.12+ ve CubeIDE ile:

```powershell
./Build.ps1
# Smartwatch SDK zaten derlenmişse:
./Build.ps1 -SkipSdkBuild
```

`Firmware/Duello.bin` uygulama başlangıcı `0x0800A000` olacak şekilde üretilir. Device Studio'daki `package_firmware.py` ile `.dfw` paketini oluşturun veya doğrulanmış `Firmware/Duello.dfw` paketini seçin. Kartta [DeviceBoot](https://github.com/polite10/DeviceBoot) kurulu olmalıdır.

```powershell
python ../DeviceStudio/package_firmware.py Firmware/Duello.bin Firmware/Duello.dfw --name Duello
```

Runtime UART: **115200 8N1**. `REQ <seq> HELLO`, `STATUS`, `BOOT` veya tablo komutları newline ile gönderilir. JSON cevap aynı sıra numarası, uygulama kimliği, bootloader bilgisi ve durum taşır. LED telemetrisi ayrı satırlardır; istemci bunları filtreler. Runtime parser sınırlı bir RX ring buffer kullanır; taşmış satırı komut olarak çalıştırmaz.

Gerçek kart testi: silahı çıkarma, ateş, meşgul eylem reddi, doldurma, yerine koyma, bağlantı zaman aşımı, reset ve Smartwatch'a dönüş; sonuçlar Device Studio'nun `hardware-report.json` dosyasındadır.

Derlemeler SDK kaynak ağacını taşımadan ortak ARM GCC yolunu kullanır. `ARM_GCC_BIN` ile araç dizini, Windows'ta `-IdeRoot` ile CubeIDE dizini seçilebilir. `--skip-sdk-build` yalnız bu kaynak sürümüyle eşleşen güncel SDK nesneleri için kullanılmalıdır; varsayılan derleme SDK'yı önce günceller.

Yerel runtime doğrulaması:

```sh
python3 tests/verify_runtime.py
```

`unicorn` ve `pyelftools` gerekir. Sonuç `build/verification-local.json` içindedir; gerçek karta erişilmez. Önceki `verification.json` / Studio `hardware-report.json` yeni derlemenin donanım testi değildir.
