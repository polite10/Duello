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

Yanına `Smartwatch` adıyla [smart_watch](https://github.com/polite10/smart_watch) checkout'u gerekir. `sdk.lock.json` doğrulanmış sürümü belirtir. ST'nin sürücüleri, mevcut kart ekran adaptörü, LVGL ve NemaGFX o repodan kullanılır; saat ekranları/modelleri Düello'ya bağlanmaz.

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
