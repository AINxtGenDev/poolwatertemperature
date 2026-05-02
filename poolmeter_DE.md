# Pool-Temperaturmonitor

Akkubetriebener ESP32 (**Wemos/Lolin LOLIN32 Lite v1.0.0**), der die Poolwassertemperatur mit einem DS18B20-Fühler misst und per MQTT über WLAN an Home Assistant meldet. Wacht alle 3 Stunden auf, überträgt die Daten in ~8 Sekunden und geht dann in Tiefschlaf. Eine einzelne geschützte NCR18650B-Zelle hält **~18 Monate** (konservativ) bis **~24 Monate** (bester Fall).

---

## ⚠ ZWINGEND VOR INBETRIEBNAHME — 3V3-Betriebs-LED auslöten

> **Zuerst erledigen. Keine Ausnahmen.**
> Der LOLIN32 Lite hat eine Betriebs-LED, die dauerhaft mit der 3V3-Schiene verbunden ist. Sie zieht **~2 mA durchgehend** — allein das ergibt ~1460 mAh pro Monat. Mit angeschlossener LED ist die 3400-mAh-Zelle nach ~2 Monaten leer; **die 6-Monats-Akkuanforderung kann nicht erfüllt werden**. Das Entfernen dieses einen SMD-Bauteils bringt die Laufzeit auf ~18 Monate.
>
> **Die andere LED auf der Platine nicht anfassen** — sie hängt an GPIO22 und wird über die Firmware gesteuert (in `main.cpp` bereits auf LOW gesetzt).

### Benötigtes Werkzeug

| Werkzeug | Hinweise |
|---|---|
| Lötkolben mit Feinspitze, 25–40 W | 1-mm-Meißel- oder Rundspitze. 320–340 °C bei bleifrei, 300 °C bei verbleitem Lot |
| ESD-sichere Pinzette, spitze Backen | Billige genügen; Hauptsache sie greift präzise |
| Entlötlitze (Kupferlitze), 1,5–2 mm | Alternativ: federgeladene Entlötpumpe |
| Flussmittelstift oder No-Clean-Flussmittelpaste | Macht das Auslöten 10× einfacher — nicht weglassen |
| Lupe, ≥ 5× | SMD-Teile sind nur ~1,6 × 0,8 mm; mit freiem Auge erkennt man keine Lötbrücken |
| Multimeter mit Durchgangsprüfer (Piepton) | Für Identifikation und Verifikation |
| Gute Beleuchtung | Tischlampe mit Tageslicht-LED dringend empfohlen |
| *(Optional)* ESD-Matte + Armband | Der ESP32 verträgt etwas Statik, aber warum das Risiko eingehen |
| *(Optional)* Isopropanol + Pinsel | Zum Entfernen der Flussmittelrückstände am Schluss |
| *(Alternative)* Scharfes Bastelmesser (X-Acto #11) | Für **Methode C** (Leiterbahn auftrennen, ohne Lötkolben) |

### Schritt 0 — Sicherheit und Vorbereitung

1. Akku abklemmen, USB abstecken. Die Platine muss beim Arbeiten stromlos sein.
2. Auf nicht leitender, hitzebeständiger Unterlage arbeiten (Silikonmatte oder Holz — kein blankes Metall).
3. Eine geerdete Metallfläche berühren, um statische Aufladung abzuleiten, bevor du die Platine anfasst.
4. Lötkolben-Schwamm anfeuchten; Spitze mit einer dünnen Schicht frischem Lot verzinnen (damit sie richtig benetzt).

### Schritt 1 — Die richtige LED identifizieren (kritisch!)

Auf der Platine sind zwei grüne LEDs. Du musst die **Betriebs-LED** (leuchtet immer bei anliegender Versorgung) auslöten und die **User-LED** (an GPIO22) in Ruhe lassen.

**Empirische Identifikation — die einzige Methode, die nicht fehlinterpretiert werden kann:**

1. USB anstecken — ob die Firmware läuft, ist egal; die 3V3-Schiene steht sofort an.
2. Bei gedämpfter Raumbeleuchtung senkrecht auf die Platine schauen.
3. **Genau eine LED leuchtet dauerhaft.** Das ist die **Betriebs-LED** — dein Ziel.
4. Die andere LED ist dunkel (oder blinkt nur kurz beim Reset). Das ist die **User-LED (GPIO22)** — NICHT anfassen.
5. USB abstecken.
6. **Ziel-LED markieren** mit einem dünnen Permanentmarker (ein kleiner Punkt neben der LED). Nicht auslassen — sobald der Lötkolben in der Hand ist, verliert man sie leicht aus dem Auge.

**Gegenkontrolle mit Multimeter (optional, aber empfohlen):**
- Multimeter in Durchgangs-/Diodenmessbereich.
- Zwischen dem Widerstands-Pad der Ziel-LED und dem 3V3-Pin des Headers messen: du solltest einen Piepton oder einen niederohmigen Pfad sehen.
- Wenn bei deiner "Ziel"-LED kein Piepton kommt, hast du die falsche LED markiert — Schritt 1 neu durchführen.

### Schritt 2 — LED auslöten (Methode A, empfohlen)

Die LED ist ein 0603-SMD-Bauteil (~1,6 × 0,8 mm) mit zwei Endkappen, die auf zwei Lötpads sitzen.

1. **Flussmittel** großzügig auf beide Endkappen auftragen. Das ist der wichtigste Einzelschritt — trockene Lötstellen lassen sich viel schlechter sauber entfernen.
2. **LED diagonal mit der Pinzette greifen**, mit leichtem Zug nach oben (gerade genug, um Widerstand zu spüren, nicht genug, um die Pads abzureißen).
3. **Lötspitze an ein Ende der LED halten**, sodass sowohl das Pad als auch die Endkappe überbrückt sind. 2–3 Sekunden halten, bis das Lot glänzend und flüssig wird.
4. **Ohne den Kolben abzuheben** die Pinzette in diese Richtung kippen, sodass das Ende der LED frei wird. Die LED sollte am noch gelöteten Ende nach oben klappen.
5. **Kolben auf das andere Ende setzen**, 2–3 Sekunden halten, die LED löst sich auf die Pinzette.
6. LED in ein Gefäß legen — nicht lose auf dem Tisch liegen lassen.
7. **Pads reinigen**: Entlötlitze auf jedes Pad drücken, mit dem Kolben 2 Sekunden berühren; die Litze saugt das Restlot auf. Für jedes Pad einen frischen Litzenabschnitt verwenden.
8. Unter Vergrößerung prüfen: beide Pads müssen flach und zinnfarben sein, **keine Lötbrücke dazwischen** und **kein abgehobenes Pad**.
9. Die Stelle mit Isopropanol abpinseln, um Flussmittelrückstände zu entfernen. Trocken tupfen.

> **Zeitbudget:** Ein ruhiger Anfänger sollte 5–10 Minuten einplanen. Wenn du länger als 30 Sekunden pro Seite kämpfst, aufhören — neu fluxen, Spitze neu verzinnen, nochmals versuchen. Wiederholte hohe Hitze hebt die PCB-Pads ab.

### Schritt 2b — Vorwiderstand auslöten (Methode B, einfachere Alternative)

Wenn du die LED selbst nicht erhitzen willst (LEDs vertragen Hitze schlecht), kannst du stattdessen den winzigen SMD-Vorwiderstand in Serie zur LED entfernen. Elektrisch äquivalent — ohne Strompfad bleibt die LED dunkel.

1. Den 0603-Widerstand finden, der direkt neben der markierten LED auf derselben Leiterbahn liegt. Er ist mit "102" (1 kΩ) oder "472" (4,7 kΩ) winzig weiß bedruckt.
2. Schritte 1–9 aus Methode A durchführen, aber den Widerstand statt der LED entfernen.
3. Widerstände lassen sich etwas leichter entfernen, weil sie Hitzeschocks problemlos vertragen.

### Schritt 2c — Leiterbahn auftrennen (Methode C, Alternative ohne Lötkolben)

Wenn du keinen Lötkolben zur Hand hast oder dich bei SMD-Rework unsicher fühlst:

1. Unter einer 5×-Lupe die **dünne Kupferbahn** finden, die die markierte LED mit ihrem Widerstand verbindet (oder den Widerstand mit der 3V3-Schiene).
2. Die Platine flach auf die Arbeitsfläche drücken. Das X-Acto-#11-Messer in steilem Winkel, fast senkrecht zur Leiterbahn, halten.
3. **Drei leichte Schnitte** quer zur Leiterbahn — nur durch den grünen Lötstopplack und die dünne Kupferschicht darunter schneiden, nicht in das FR4-Glasfasermaterial eindringen.
4. Mit einer Nadel einen kleinen Spalt zwischen den beiden Leiterbahnenden herauskratzen (Breite ≥ 0,3 mm).
5. Mit dem Multimeter im Durchgangsbereich verifizieren: Messspitzen auf beide Seiten des Schnitts — muss **offen** anzeigen (kein Piepton).

> Methode C ist unumkehrbar und sieht unschön aus, ist aber 100 % zuverlässig, wenn du dich mit dem Lötkolben unsicher fühlst.

### Schritt 3 — Reparatur überprüfen

1. USB anstecken. **Die Betriebs-LED muss dunkel bleiben.** Wenn sie noch leuchtet, hast du entweder die falsche LED entfernt oder Methode A hat eine Lötbrücke hinterlassen — nochmals nachsehen.
2. Multimeter in µA-Bereich (Mikroampere), in Serie auf der JST-(+)-Leitung zwischen Keystone-Halter und LOLIN32 Lite schalten.
3. Firmware in Tiefschlaf gehen lassen (erster Wach-Zyklus abgeschlossen, dann ~3-Stunden-Timer aktiv).
4. **Strom ablesen: muss < 120 µA sein.** Nach korrekter LED-Entfernung sind 80–100 µA typisch.
5. Liegt der Wert über ~500 µA, wird die Betriebs-LED irgendwo noch gespeist — Schritte 1 und 2 neu prüfen.
6. Gemessenen Strom in `SESSION_CHECKPOINT.md` unter "Open items → bench test" für die Nachvollziehbarkeit festhalten.

### Schritt 4 — Dokumentieren und schützen

- Eine Nahaufnahme der Platine mit entfernter LED / Widerstand machen. An die Bauunterlagen anhängen.
- Einen Tupfer klaren Nagellack oder UV-Harz über die bearbeitete Stelle geben, um sie gegen Pool-Feuchte zu versiegeln (optional, aber das Gerät steht im Freien).

---

## Inhaltsverzeichnis

1. [Energiebilanz](#energiebilanz)
2. [Stückliste](#stückliste)
3. [Bezugsquellen (Österreich / DACH)](#bezugsquellen-österreich--dach)
4. [Verdrahtungsplan](#verdrahtungsplan)
5. [Platinenlayout](#platinenlayout-5×7-cm-hand-lötplan)
6. [Firmware-Einrichtung](#firmware-einrichtung)
7. [Quellcode](#quellcode)
   - [platformio.ini](#platformioini)
   - [config.h](#configh)
   - [main.cpp](#maincpp)
8. [Home-Assistant-Einrichtung](#home-assistant-einrichtung)
   - [MQTT-Konfiguration](#mqtt-konfiguration-optional)
   - [Automatisierungen](#automatisierungen)
   - [Dashboard-Karten](#dashboard-karten)
9. [Gehäuse & Wasserdichtigkeit](#gehäuse--wasserdichtigkeit)
10. [Optimierungstipps](#optimierungstipps)
11. [Fehlersuche](#fehlersuche)

---

## Energiebilanz

Die Werte gelten für den **LOLIN32 Lite v1.0.0 mit ausgelöteter 3V3-Betriebs-LED**. Wird die LED-Modifikation übersprungen, läuft die Entladung 20× schneller und die Zelle ist nach ~2 Monaten leer.

| Parameter | Wert |
|---|---|
| Akku | NCR18650B, 3,7 V, 3400 mAh (geschützt) |
| Nutzbare Kapazität (80 % DoD + LDO-Verluste) | ~2720 mAh |
| Tiefschlaf-Strom (ESP32 + ME6211-Iq + Leckstrom, LED entfernt) | ~90 µA |
| Aktivstrom (ESP32 + WLAN-TX Mittelwert) | ~200 mA |
| DS18B20 (12-Bit-Wandlung) | ~1 mA für 750 ms |
| Dauer eines Wach-Zyklus | ~8 s |
| Messungen pro Tag (alle 3 h) | 8 |

| Tagesverbrauch | Wert |
|---|---|
| Tiefschlaf (23 h 59 min × 90 µA) | 2,16 mAh |
| Aktivphasen (8 × 8 s × 200 mA) | 3,56 mAh |
| **Gesamt** | **~5,7 mAh/Tag** |

| Prognose | Wert |
|---|---|
| 6 Monate (180 Tage) | 1030 mAh (30 % der Kapazität) |
| **Geschätzte Laufzeit** | **~575 Tage (~18 Monate)** |

> Die 6-Monats-Anforderung ist mit 3× Sicherheitsreserve erfüllt. Der CH340 USB-UART-Chip zieht nur ~10 mA, wenn ein USB-Kabel angesteckt ist (also beim Laden / Flashen); auf die Feldlaufzeit hat er keinen Einfluss.

---

## Stückliste

Aufgebaut um das **LOLIN32 Lite v1.0.0**, das du bereits besitzt. Alle Zusatzkomponenten sind **bedrahtet / von Hand lötbar**.
Der LOLIN32 Lite selbst ist werkseitig vorbestückt — du musst kein SMD-Bauteil von Hand löten.

| # | Anz. | Komponente | Spezifikation / exaktes Teil | Zweck | ~EUR |
|---|---|---|---|---|---|
| 1 | 1 | **Wemos/Lolin LOLIN32 Lite v1.0.0** | ESP32-D0WDQ6, ME6211-33 LDO, TP4054 USB-Lader, CH340 USB-UART, JST-1,25-mm-PH-Akkustecker | MCU + WLAN + Ladeschaltung | (vorhanden) |
| 2 | 1 | DS18B20 wasserdichter Fühler | 1 m Edelstahl, 3-adrig (rot/schwarz/gelb), IP68 | Wassertemperatur | 4,50 |
| 3 | 1 | NCR18650B **geschützt** 3400 mAh | Panasonic/Sanyo, Button-Top, mit Schutzbeschaltung (PCM) | Hauptakku | 7,00 |

| 4 | 1 | Keystone-1042P-Halter | bedrahtet, passt für 69-mm-Schutzzellen | Akkuhalter | 1,80 |
| 5 | 1 | **JST-PH-1,25-mm-2-pol.-Pigtail**, ≥ 10 cm | Stecker + blanke Litzen (rot/schwarz) | Keystone → JST-Eingang des LOLIN32 Lite verbinden | 0,30 |
| 6 | 1 | 1N5817 Schottky-Diode, DO-41 | 1 A / 20 V, Vf ≈ 0,3 V | Verpolschutz im Pigtail | 0,15 |
| 7 | 1 | 4,7-kΩ-Widerstand, 1/4 W | beliebig | DS18B20-DATA-Pull-up | 0,05 |
| 8 | 2 | 1-MΩ-Widerstand, 1/4 W, 1 % Metallfilm | geringe Drift | Akku-Spannungsteiler (~2,1 µA Ruhestrom) | 0,10 |
| 9 | 2 | 100 nF radiale Keramik, 50 V | X7R | ADC-Filter (Teilerabgriff → GND), DS18B20-VDD-Bypass | 0,10 |
| 10 | 1 | Streifenrasterplatine, 5×7 cm | FR4 einseitig, 2,54-mm-Raster | Handlöt-Plattform | 1,00 |
| 11 | 2 | Buchsenleiste, 2,54 mm, 1×16 | auf Länge brechen | Sockel für LOLIN32 Lite (tauschbar) | 0,50 |
| 12 | 1 | Verzinnter Schaltdraht, 0,5 mm², 2–3 m | | Interne Verdrahtung | 1,00 |
| 13 | 1 | IP67 ABS-Gehäuse, ~100 × 68 × 50 mm | Hammond 1554C oder generisch | Gehäuse | 4,50 |
| 14 | 1 | PG7-Kabelverschraubung, grau | 3–6,5 mm Kabel | Fühler-Durchführung | 0,80 |
| 15 | 1 | Neutrales Silikon (Tube) | | Abdichtung Verschraubung + interne Zugentlastung | 2,00 |
| 16 | 1 | Schrumpfschlauch-Sortiment, 2–6 mm | | Schutz der Fühlerlötstellen | 1,00 |
|   |   | **Pro Gerät (ohne Board)** |   |   | **~23 EUR** |

### Beim LOLIN32 Lite nicht mehr nötig (aus Originalplan entfernt)

| Teil | Grund für Entfernung |
|---|---|
| HT7333-A LDO | Onboard-ME6211-33 übernimmt die Regelung (Iq ≈ 50 µA) |
| 2× 1 µF + 2× 10 µF LDO-Entkoppelkondensatoren | Auf der LOLIN32-Lite-Platine bereits vorhanden |
| 10 kΩ EN-Pull-up + 100 nF EN-Entprellung | Bereits onboard |
| 10 kΩ GPIO0-Pull-up + BOOT-Taster | Erledigt durch CH340 DTR/RTS-Auto-Reset + onboard-Pull-up |
| Reset-Taster | Onboard-**RST**-Taster ist vorhanden |
| CP2102 USB-TTL-Adapter | CH340 onboard — direkt über den Micro-USB-Port flashen |
| TP4056-Lademodul | TP4054 bereits onboard (lädt über Micro-USB) |

### Verpflichtende Boardmodifikationen vor Inbetriebnahme

1. **3V3-Betriebs-LED physisch entfernen** auf dem LOLIN32 Lite. Die kleine grüne LED neben dem "3V3"-Header-Pin — NICHT die LED in der Nähe von GPIO22. Optionen (eine wählen):
   - Mit Feinspitz-Lötkolben + Pinzette auslöten.
   - Den Vorwiderstand (danebenliegender SMD-0603-Widerstand) mit einem Bastelmesser durchkratzen.
   - Mit Multimeter prüfen: die LED muss dunkel sein und der Widerstand muss offen (kein Durchgang) messen.
2. **Tiefschlaf-Strom testen** auf der BAT+-Seite der JST. Zielwert: **< 120 µA**. Wenn höher, ist die LED-Modifikation unvollständig oder eine Lötbrücke vorhanden.

### Optionale Erweiterungen

- **Externe 2,4-GHz-Antenne** — **auf dieser Platine nicht möglich** (nur PCB-Leiterbahnantenne, kein u.FL/IPEX-Anschluss). Falls am Pool-Standort das WLAN schwach ist, muss ein ESP32-WROOM-32**U** eingesetzt werden. Vor der Endmontage eine RSSI-Messung am Pool durchführen.
- **Solarpanel + TP4054-Eingang** — machbar über den Micro-USB-Port (5 V, 500 mA max.), aber bei der berechneten ~18-Monats-Laufzeit selten sinnvoll.

---

## Bezugsquellen (Österreich / DACH)

Da du den LOLIN32 Lite schon hast, passt alles Weitere in eine einzelne **Reichelt**-Bestellung (Versand nach Österreich, Pauschale ~5,95 €, 2–3 Werktage).

### Haupt-Warenkorb — Reichelt.de → Österreich

| SL # | Artikel | Reichelt-Such-URL |
|---|---|---|
| 2 | DS18B20 wasserdichter Fühler | https://www.reichelt.de/at/de/shop/produkt/temperatursensor_digital_1-wire_ds18b20-230413 |
| 3 | NCR18650B geschützt 3400 mAh | https://www.reichelt.de/at/de/shop/produkt/lithium-ionen_akku_18650_3_7_v_3_400_mah-251999 |
| 4 | Keystone 1042P 18650-Halter | https://www.reichelt.de/at/de/shop/produkt/batteriehalter_keystone_18650_print-keystone_1042-214290 |
| 5 | JST-PH-1,25-mm-2-pol.-Pigtail | Seitensuche `jst ph 1,25 mm kabel 2-polig` (als Pigtail-Packungen, oft 10er) |
| 6 | 1N5817 Schottky DO-41 | https://www.reichelt.de/at/de/shop/produkt/schottky-diode_1n_5817-1n_5817-14840 |
| 7–9 | Passive (4,7 kΩ, 1 MΩ × 2, 100 nF × 2) | Seitensuche `1/4W metallfilm 1MΩ`, `100nF 50V X7R radial`. Alle < 0,10 € pro Stück |
| 10 | Streifenrasterplatine 5×7 cm | Seitensuche `streifenrasterplatine 5x7` |
| 11 | Buchsenleiste 1×16, 2,54 mm | Seitensuche `buchsenleiste 1x16 gerade` |
| 13 | IP67 ABS-Gehäuse ~100 × 68 × 50 | Seitensuche `gehäuse abs ip67 100` |
| 14 | PG7-Kabelverschraubung | Seitensuche `pg7 kabelverschraubung` |

### Alternativen / Spezialartikel

| Bedarf | Shop | Grund |
|---|---|---|
| Ersatz-LOLIN32 Lite | **Amazon.at** — Suche `LOLIN32 Lite V1.0.0` | ~8–10 €, Prime am nächsten Tag. Sicherstellen, dass im Angebot ESP32-WROOM-32 / ESP32-D0WDQ6 steht — "LOLIN S2 Mini" meiden (anderer Chip, andere Pin-Belegung) |
| Abholung am selben Tag in Wien/Graz/Linz/Salzburg/Innsbruck | **Conrad.at** (`https://www.conrad.at/`) | Filialabholung für passive Bauteile |
| Original Panasonic NCR18650B | **nkon.nl** oder **akkuteile.de** | Fälschungen sind auf Amazon/AliExpress häufig |
| JST-PH-1,25-mm-Pigtail-Sortimente | **TME.eu** | Verlässlicher Lagerbestand, falls bei Reichelt aus |
| Originale KEMET-/Nichicon-Teile | **Mouser.at** / **Distrelec.at** | Nur nötig bei spezieller Toleranz / Lebensdauerklasse |

### Geschätztes Versandbudget

- Reichelt-Einzelbestellung: ~23 € Teile + 5,95 € Versand = **~29 € gesamt**.
- (Du hast das Board bereits, daher kein Board-Posten.)

---

## Verdrahtungsplan

> **Wer das hier zum ersten Mal aufbaut, liest diesen Abschnitt Schritt für Schritt durch.** Die Schaltung besteht aus nur **vier Baugruppen**, die nacheinander verdrahtet werden. Jede Baugruppe hat eine eigene kleine Zeichnung und eine Tabelle.

Der LOLIN32 Lite ist ein "fertig eingerichtetes" Board: Spannungsregelung, USB-Laden, Reset- und Boot-Logik sind schon drauf. Du baust nur drei Dinge außen herum:

1. **Akku + Verpolschutz** → füttert das Board mit Strom
2. **Temperaturfühler DS18B20** → misst die Wassertemperatur
3. **Spannungsteiler** → zeigt, wie voll der Akku ist
4. **Micro-USB** → zum Flashen und zum Nachladen (nichts zu löten)

---

### Übersicht — so sieht alles zusammen aus

```
   ┌────────────┐                  ┌──────────────────────┐
   │   18650    │  (1) Akku + Di-  │                      │◄── (4) Micro-USB
   │  Keystone  │──── ode ────────►│   LOLIN32 Lite       │    (Flashen + Laden)
   │   Halter   │                  │   v1.0.0             │
   └────────────┘                  │  (Betriebs-LED raus) │
                                   │                      │
                                   │                      │────► (2) DS18B20
                                   │                      │       Temperaturfühler
                                   │                      │
                                   │                      │────► (3) Spannungsteiler
                                   │                      │       (misst Akku)
                                   └──────────────────────┘
```

**Farbcode der Drähte** (hilft beim Nachverfolgen):
- 🔴 **Rot** = Plus (+) / Versorgung
- ⚫ **Schwarz** = Minus (−) / GND / Masse
- 🟡 **Gelb** = Datenleitung
- 🟢 **Grün/Weiß** = Signal / Messleitung

---

### Pin-Karte des LOLIN32 Lite (was tatsächlich auf dem Board steht)

> **Wichtig für Einsteiger:** Wenn im Rest des Dokuments "GPIO16", "GPIO4", "GPIO35" usw. steht, findest du auf dem Board **nur die Zahl** aufgedruckt — also `16`, `4`, `35`. Das "GPIO" ist das technische Präfix aus dem Datenblatt und steht nicht auf der Platine. Namen wie **VDD**, **DATA**, **GND** stehen dagegen nicht auf dem Board, sondern **nur auf dem Sensor oder im Datenblatt**.

Vorderseite des LOLIN32 Lite v1.0.0 (Antenne oben):

```
                  ┌─── PCB-Antenne ───┐
                  │                   │
       Links      │                   │     Rechts
     (von oben              nach unten):
       VP  ●──                          ──● 3V      ← 3,3 V (geregelt)
       VN  ●──                          ──● 22      ← nicht verwenden (User-LED)
       EN  ●──                          ──● 18
       34  ●──                          ──● 23
       35  ●──    ← hier kommt der      ──● 17
                  Spannungsteiler hin
       32  ●──                          ──● 16      ← hier kommt 🔴 (Fühler-Rot) hin
       25  ●──                          ──●  4      ← hier kommt 🟡 (Fühler-Gelb) hin
       26  ●──                          ──●  0
       27  ●──                          ──●  2
       14  ●──                          ──● 15
       12  ●──                          ──● 13
       GND ●──                          ──●
     (oder  6)

                [JST-Stecker]  [RST]  [Micro-USB]
                  Akku-          Reset-
                  Anschluss      Taster
```

**So liest du diese Karte:**
- Nummern wie `16` oder `4` → im Dokument mit **"GPIO" davor** geschrieben (`GPIO16`, `GPIO4`).
- `3V` auf dem Board = im Dokument **"3V3"** geschrieben (3,3 V geregelt).
- `GND` ist GND — beides heißt Masse.
- `22` bitte **frei lassen** (ist die onboard User-LED, die Firmware steuert sie selbst).

---

### Baugruppe 1 — Akku anschließen (mit Verpolschutz)

Das ist der Strompfad vom Akku zum Board. Die Diode **1N5817** schützt das Board, falls der Akku versehentlich falsch herum in den Halter kommt.

```
     Keystone 1042P                       LOLIN32 Lite
     (18650-Halter)                       JST-Stecker (1,25 mm)
    ┌──────────────┐                        ┌────────┐
    │              │                        │        │
    │   (+) ●──────┤──🔴──►│◄──🔴─────────┤ (+)    │
    │              │   (1N5817 Schottky)    │        │
    │              │   Ring/Strich = KATHODE (zum Board hin!)
    │              │                       │        │
    │   (−) ●──────┤──────────⚫──────────┤ (−)    │
    │              │                       │        │
    └──────────────┘                       └────────┘
```

| Von (Keystone) | Draht | Über | Nach (LOLIN32 JST) |
|---|---|---|---|
| **(+)** Plus-Pol | 🔴 Rot | **Diode 1N5817**, Silberring zum Board | **(+)** JST-Plus |
| **(−)** Minus-Pol | ⚫ Schwarz | *(direkt)* | **(−)** JST-Minus |

> **Merksatz zur Diode:** Der Silberring (oder schwarze Strich) auf der Diode muss Richtung **Board** zeigen. Falsch herum → das Board bekommt keinen Strom. Alle Lötstellen mit Schrumpfschlauch isolieren.

---

### Baugruppe 2 — Temperaturfühler DS18B20 (3 Drähte)

Der Fühler hat drei Adern: **Rot** (Versorgung), **Schwarz** (Masse), **Gelb** (Daten). Dazu kommt **ein** Pull-up-Widerstand 4,7 kΩ und **ein** Stütz-Kondensator 100 nF.

> **Wichtig zur Beschriftung:** Auf dem Board siehst du **nur Zahlen** (`16`, `4`) und `GND` / `3V`. Die Namen **VDD**, **DATA** und **GND** gehören zum **DS18B20-Fühler** (stehen so im Datenblatt des Sensors — nicht auf dem Board!). Der rote Draht geht also **vom Board-Pin `16`** zum **Sensor-Pin "VDD"**.

```
   LOLIN32 Lite                              DS18B20 (Fühler)
  ┌──────────────┐                            ┌──────────────┐
  │              │                            │              │
  │  Pin "16" ●─┤──── 🔴 rot ────────┬──────►│ VDD          │
  │  (= GPIO16)  │                    │       │              │
  │              │                  ┌─┴─┐     │              │
  │              │                  │100│     │              │
  │              │                  │ nF│     │              │
  │              │                  └─┬─┘     │              │
  │              │                    │       │              │
  │  Pin "GND"●─┤───── ⚫ schwarz ───┴──────►│ GND          │
  │              │                            │              │
  │              │                            │              │
  │  Pin "4" ●──┤───── 🟡 gelb ──────┬──────►│ DATA         │
  │  (= GPIO4)   │                    │       │              │
  │              │                [4,7 kΩ]    └──────────────┘
  │              │                   │
  │  Pin "3V" ●─┤───────────────────┘
  │  (= 3V3)     │       (Pull-up-Widerstand)
  └──────────────┘
```

| Board-Pin *(Aufdruck)* | Draht-Farbe | Bauteil dazwischen | Sensor-Pin *(Datenblatt-Name)* |
|---|---|---|---|
| **`16`** (GPIO16) | 🔴 Rot | *(direkt)* + **100 nF** von dieser Leitung nach GND | **VDD** (roter Fühlerdraht) |
| **`GND`** | ⚫ Schwarz | *(direkt)* | **GND** (schwarzer Fühlerdraht) |
| **`4`** (GPIO4) | 🟡 Gelb | *(direkt)* | **DATA** (gelber Fühlerdraht) |
| **`3V`** (3V3) | (kurze Brücke) | **4,7-kΩ-Widerstand** | an `4`/DATA-Leitung — **nicht direkt zum Sensor** |

**Was die Teile tun (in einem Satz):**
- **4,7-kΩ-Widerstand** = "zieht" die Datenleitung auf HIGH, wenn keiner spricht. **Pflicht**, sonst kommen nur `−127 °C`.
- **100-nF-Kondensator** = Stützkondensator, glättet kurze Spannungsdellen am Fühler.

---

### Baugruppe 3 — Akku-Spannungsteiler (misst, wie voll der Akku ist)

Der ESP32 darf maximal 3,3 V an seinem Analog-Pin sehen, der Akku kann aber bis zu 4,2 V liefern. Zwei gleich große Widerstände **halbieren** die Spannung, der ESP32 rechnet dann intern wieder ×2.

```
   JST(+) ───────────────┐
   (hinter der Diode,    │
    ungeregelte          │
    Akkuspannung)        │
                         │
                       [1 MΩ]    ← Widerstand #1
                         │
                         ├──────────────► GPIO35
                         │                 (Mess-Eingang)
                         │
                         ├──[100 nF]── GND    (Stütz-Kondensator)
                         │
                       [1 MΩ]    ← Widerstand #2
                         │
                        GND
```

| Verbindung | Von | Nach |
|---|---|---|
| oberes Ende Widerstand #1 | **JST(+)** auf der Rückseite des Boards *(hinter der Diode)* | 1-MΩ-Widerstand #1 |
| Mittelpunkt (beide Widerstände treffen sich hier) | zwischen den beiden 1-MΩ-Widerständen | **GPIO35** *(und* **100 nF** *nach GND)* |
| unteres Ende Widerstand #2 | 1-MΩ-Widerstand #2 | **GND** |

> **Wichtig:** Das obere Ende kommt **nicht** an den 3V3-Pin, sondern an den **JST-Plus** (die ungeregelte Akkuspannung). Nur dort sieht der Spannungsteiler den echten Akkustand. Das passende Lötpad liegt auf der **Rückseite** des LOLIN32 Lite, direkt hinter dem JST-Stecker.

---

### Baugruppe 4 — Micro-USB (nichts zu verdrahten)

Der **Micro-USB-Port** auf dem Board macht zwei Dinge gleichzeitig:
- **Flashen** der Firmware (PC → ESP32)
- **Akku laden** (der onboard-TP4054 lädt die 18650-Zelle automatisch mit 500 mA)

Einfach Micro-USB-Kabel einstecken. Kein Löten nötig.

---

### Gesamt-Pin-Übersicht (Spickzettel für die Montage)

> Spalte "Aufdruck am Board" = das, was **wirklich** auf dem LOLIN32 Lite steht.
> Spalte "Im Code / Datenblatt" = wie der Pin in der Firmware oder im Sensor-Datenblatt heißt.

| Aufdruck am Board | Im Code / Datenblatt | Das kommt dran | Farbe |
|---|---|---|---|
| **JST (+)** | VBAT | Akku-Plus über **Diode 1N5817** | 🔴 |
| **JST (−)** | GND | Akku-Minus | ⚫ |
| **`16`** | GPIO16 | roter Fühlerdraht (geht am Sensor auf **VDD**) + 100 nF nach GND | 🔴 |
| **`4`** | GPIO4 | gelber Fühlerdraht (geht am Sensor auf **DATA**) + 4,7 kΩ nach `3V` | 🟡 |
| **`GND`** | GND | schwarzer Fühlerdraht (am Sensor **GND**), Teiler-Unterseite, alle Kondensatoren | ⚫ |
| **`3V`** | 3V3 | nur als Anker für den 4,7-kΩ-Pull-up | — |
| **`35`** | GPIO35 | Mitte des Spannungsteilers (+ 100 nF nach GND) | 🟢 |
| **`22`** | GPIO22 | **nichts anschließen** — onboard User-LED (Firmware steuert sie) | — |
| **`RST`** | EN/Reset | **nichts anschließen** — Taster ist schon aufgelötet | — |
| **Micro-USB** | — | einfaches Kabel zum PC oder Ladegerät | — |

---

### Vorher abhaken — die drei Pflicht-Punkte

- [ ] **3V3-Betriebs-LED ist ausgelötet** (siehe Abschnitt ganz oben). Ohne diesen Schritt hält der Akku keine 2 Monate.
- [ ] **Diode 1N5817 richtig herum** eingebaut — Silberring zeigt zum Board.
- [ ] **Tiefschlaf-Strom gemessen**: Multimeter in µA-Bereich in Serie mit der JST(+)-Leitung — Anzeige **< 120 µA**. Ist der Wert höher, glimmt die LED-Modifikation oder eine Lötbrücke zieht Strom.

---

### Warum ist das so aufgebaut? (Kurz-FAQ für Nicht-Profis)

**Warum die Diode 1N5817 im Akkukabel?**
> Falls der Akku einmal verdreht in den Halter rutscht, fließt trotzdem kein Strom ins Board. Die Diode "opfert" sich lieber selbst. Sie kostet nur 0,3 V — das Board läuft damit problemlos weiter.

**Warum 1-MΩ-Widerstände für den Teiler (nicht 100 kΩ)?**
> Kleinere Widerstände würden dauerhaft ~21 µA durchs Board ziehen — über ein Jahr gerechnet ~180 mAh verschenkt. Mit 1 MΩ sind es nur 2 µA. Den Nachteil (träge Messung) fängt der 100-nF-Kondensator ab.

**Warum den Fühler aus GPIO16 versorgen statt aus 3V3?**
> Im Tiefschlaf schaltet der ESP32 GPIO16 ab → der Fühler verbraucht **nichts**. Würde er an 3V3 hängen, würde er dauerhaft ~1 mA ziehen und den Akku in wenigen Monaten leer saugen.

**Warum nicht an VIN anschließen?**
> "VIN" auf der Rückseite ist ein **USB-5-V-Abgriff**, nicht der Akku-Eingang. Der Akku gehört **immer an den JST-Stecker**. Dort sitzt die richtige Ladeschaltung.

---

### Flashen — was tun, wenn's nicht klappt

1. **Normalfall**: Micro-USB einstecken, in PlatformIO `Upload` klicken → fertig. Der CH340-Chip auf dem Board löst den Reset selbst aus.
2. **Notfall** (wenn "Connecting…" endlos läuft):
   - **GPIO0** mit einer Drahtbrücke auf **GND** ziehen
   - Den **RST**-Taster auf dem Board kurz drücken
   - Drahtbrücke wieder entfernen
   - `Upload` nochmal klicken

### Wichtige Verbindungen

| LOLIN32-Lite-Pin | Verbunden mit | Zweck |
|---|---|---|
| JST (+) | Keystone BAT+ **über 1N5817 Anode→Kathode** | Zelle → onboard-Lader + ME6211 |
| JST (−) | Keystone BAT− | Gemeinsame Masse |
| GND (Header) | DS18B20 GND, Teiler-Unterseite, 100-nF-Kondensatoren | Gemeinsame Masse |
| 3V3 (Header) | 4,7-kΩ-Pull-up (oben) | DS18B20-DATA-Pull-up |
| GPIO4 | DS18B20 DATA (gelb) + 4,7 kΩ → 3V3 | OneWire-Datenbus |
| GPIO16 | DS18B20 VDD (rot) + 100 nF → GND | Versorgt den Sensor nur in der Aktivphase |
| GPIO35 | 1-MΩ-/-1-MΩ-Teiler-Mittelabgriff + 100 nF → GND | Akkuspannung (ADC1_CH7, nur Eingang) |
| GPIO22 | *(nicht verwenden — onboard User-LED)* | In der Firmware auf LOW gesetzt |
| RST | *(onboard-Taster)* | Hardware-Reset |

### Warum die 1N5817 im Pigtail?

Falls die Zelle jemals verpolt in den Keystone-Halter eingesetzt wird, schützt die Schottky sowohl den TP4054-Lader als auch den ME6211-Regler vor Verpolspannung. Die Flussspannung von ~0,3 V ist unkritisch, weil:
- Der TP4054 ≥ 4,25 V am Eingang braucht (von USB, nicht von der Zelle — die Diode liegt also nicht im Ladepfad *von USB aus*; sie liegt nur im Pfad *von der Zelle Richtung Regler*, wo 3,7 V − 0,3 V = 3,4 V den ME6211-Dropout weiterhin überschreitet).
- Der ME6211-Dropout nur 90 mV bei 100 mA beträgt, also 3,4 V Eingang mit Reserve immer noch 3,3 V Ausgang ergeben.

### Warum 1 MΩ / 1 MΩ Teiler (nicht 100 kΩ / 100 kΩ)?

Ein 100-kΩ-/-100-kΩ-Teiler zieht 4,2 V ÷ 200 kΩ ≈ **21 µA dauerhaft** — das sind grob **180 mAh/Jahr**, ein bedeutender Teil der Zellkapazität, nur für die Messung verschwendet.
 Ein 1-MΩ-/-1-MΩ-Teiler zieht nur **2,1 µA** (≈ 18 mAh/Jahr). Der Kompromiss ist eine hohe Quellimpedanz, die der ESP32-ADC (~130 kΩ Eingang) nicht schnell treiben kann — die
 **100 nF von ADC-Pin nach GND** halten die Spannung am Mittelabgriff während der 32-Sample-Messung stabil.

### Warum den DS18B20 aus einem GPIO speisen?

Im Tiefschlaf gehen alle GPIOs in einen definierten Zustand (LOW / isoliert). Das trennt die Versorgung des DS18B20 und spart seinen Ruhestrom von ~1 mA. Über 6 Monate sind das ~4320 mAh — mehr als die gesamte Akkukapazität.

### Warum den Akku nicht an VIN anschließen?

Der LOLIN32 Lite erwartet den Akku-Eingang **am JST-Stecker** — dieser Pfad speist sowohl den TP4054-Lader als auch den ME6211-Regler. Das auf der Rückseite mit "VIN" beschriftete
 Pad ist ein USB-5-V-Abgriff und **nicht** der korrekte Eingang für eine einzelne Zelle. Immer die JST-Seite verwenden.

---

## Platinenlayout (5×7 cm, Hand-Lötplan)

Streifenrasterplatine mit 2,54-mm-Raster, Kupferstreifen verlaufen horizontal. Streifen auftrennen, wo mit **✂** markiert. Zeilennummern = Streifen von oben. Spalten 1–18, Zeilen 1–17.

```
 Zeile  1   (3V3-Schiene) : ●══════════════════════════════════════════════●
 Zeile  2   (GND-Schiene) : ●══════════════════════════════════════════════●
 Zeile  3                  :
 Zeile  4   Lolin L-Spalte: 3V3  22  19  23  18   5  17  16   4   0   2  15  13
 Zeile  5                  : (LOLIN32 Lite in Zeilen 4 + 8 auf 1×16-Buchsenleisten gesockelt)
 Zeile  6                  :
 Zeile  7                  :
 Zeile  8   Lolin R-Spalte: GND  12  14  27  26  25  33  32  35  VN  VP  EN
 Zeile  9                  :
 Zeile 10   DS18B20        : [●── gelb (DATA) ──●]   3-polige Schraubklemme oder Pads
 Zeile 11   Fühler-Streifen: [●── rot  (VDD)  ──●]
 Zeile 12                  : [●── sw   (GND)  ──●]
 Zeile 13   Teiler         :                                  [1MΩ]────► zu GPIO35
 Zeile 14                  :                                  [1MΩ]────► zu GND
 Zeile 15   JST-Pigtail    : (+)──▷|── (1N5817) ──► Lolin JST (+)     (−) ── Lolin JST (−)
 Zeile 16                  :
 Zeile 17                  :

 Markierungen zum Auftrennen (✂):
   Zwischen Zeile 1 und 2 auf jeder vertikalen Bahn, die etwas trägt — Standard:
   KEINE Schnitte an den 3V3- und GND-Schienen selbst.
   Unter dem LOLIN32-Sockel: jede Leiterbahn zwischen linker und rechter Spalte
   auftrennen, damit 3V3/22/19/… getrennte Netze bleiben. (Zwischen Loch 1 und 2
   jeder Zeilenlücke unter dem Sockelkörper.)

 Verdrahtungsbrücken (isolierter Schaltdraht, so kurz wie möglich):
   Lolin 3V3   → Schiene 1
   Lolin GND   → Schiene 2
   Lolin GPIO4 → DS18B20-DATA-Streifen (Zeile 10); 4,7 kΩ von diesem Knoten zu Schiene 1
   Lolin GPIO16→ DS18B20-VDD-Streifen (Zeile 11); 100 nF von diesem Knoten zu Schiene 2
   Lolin GND   → DS18B20-GND-Streifen (Zeile 12)
   Lolin GPIO35→ Teiler-Mittelabgriff (Verbindungspunkt der beiden 1 MΩ); 100 nF zu Schiene 2
   Teiler-Oberseite (oberes Ende von 1 MΩ #1) → Lolin-JST(+)-Netz (nicht mit der 3V3-
     Schiene verbinden — es geht um die ungeregelte Zellspannung; diesen Draht
     auf das JST-(+)-Lötpad auf der Rückseite des Boards ziehen, hinter der Diode)
   Teiler-Unterseite (unteres Ende von 1 MΩ #2) → Schiene 2 (GND)
```

**Lötreihenfolge (reduziert Fehler):**
1. Zuerst **die Kupferstreifen unter dem Sockelbereich auftrennen** (einen 3-mm-Bohrer von Hand in jede Trennstelle drehen — schneller als mit dem Messer).
2. **Die beiden 1×16-Buchsenleisten zuerst einlöten** — sie definieren die LOLIN32-Position. Pin-Raster mit Trockenanpassung doppelt prüfen.
3. Den **4,7-kΩ**-Pull-up, die **beiden 1-MΩ-Teiler-Widerstände** und die **beiden 100-nF**-Kondensatoren einlöten. Anschlussdrähte kurz halten.
4. Die **3-polige DS18B20-Schraubklemme** einlöten (oder die Fühlerkabelenden direkt auf der Platine mit Zugentlastung anlöten).
5. Den **Akku-Pigtail außerhalb der Platine** zusammenbauen:
   - Schwarze Litze des JST-Pigtails trennen; mit Keystone (−) spleißen.
   - Rote Litze des JST-Pigtails trennen; **1N5817** in Serie einfügen, Anode Richtung Keystone (+), Kathode Richtung JST (+). Jede Verbindung doppelt mit Schrumpfschlauch sichern.
6. **Vor** Einsetzen der Zelle: LOLIN32 Lite in den Sockel stecken, mit 4,0 V Labornetzgerät an JST (+) / (−) versorgen, prüfen dass der 3V3-Pin 3,30 V ± 0,05 V zeigt. Leerlaufstrom < 120 µA bestätigen.
7. Firmware via Micro-USB flashen, dann Tiefschlaf-Strom auf der BAT+-Leitung erneut messen. **Zielwert: < 120 µA.** Ist er höher, leuchtet die 3V3-Betriebs-LED noch irgendwo.

---

## Firmware-Einrichtung

### Voraussetzungen

- [PlatformIO](https://platformio.org/install) (VS-Code-Erweiterung oder CLI)
- USB-Kabel für das initiale Flashen

### Schritte

1. **Das Verzeichnis `firmware/` klonen oder kopieren.**

2. **Konfiguration bearbeiten** in `firmware/include/config.h`:
   ```cpp
   #define WIFI_SSID      "dein_wlan_ssid"
   #define WIFI_PASSWORD   "dein_wlan_passwort"
   #define MQTT_SERVER     "192.168.1.100"  // Home Assistant IP
   #define MQTT_USER       "mqtt_user"
   #define MQTT_PASSWORD   "mqtt_pass"
   ```

3. **Bauen und flashen**:
   ```bash
   cd firmware
   # Für ESP32:
   pio run -e esp32 -t upload

   # Für ESP8266:
   pio run -e esp8266 -t upload
   ```

4. **Seriellen Output überwachen** (optional, zum Debuggen):
   ```bash
   pio device monitor -b 115200
   ```

5. **USB abstecken**, Akku anschließen, Gerät einsetzen.

### Ablauf pro Wach-Zyklus

1. ESP32 wacht aus Tiefschlaf auf
2. GPIO16 geht auf HIGH → versorgt den DS18B20
3. Temperaturwandlung startet (750 ms)
4. WLAN verbindet sich parallel (~2–5 s)
5. Temperatur wird vom DS18B20 gelesen
6. Akkuspannung wird per ADC gelesen
7. MQTT-Discovery-Konfiguration wird (retained) veröffentlicht
8. Temperatur- und Akkudaten werden veröffentlicht
9. WLAN und Sensor werden abgeschaltet
10. ESP32 geht für 3 Stunden in Tiefschlaf

---

## Quellcode

### platformio.ini

```ini
; PlatformIO Configuration for Pool Temperature Monitor
; =====================================================
; Primary target: Wemos/Lolin LOLIN32 Lite v1.0.0.

[env]
framework = arduino
monitor_speed = 115200
lib_deps =
    paulstoffregen/OneWire@^2.3.8
    milesburton/DallasTemperature@^3.11.0
    knolleary/PubSubClient@^2.8
build_flags =
    -DMQTT_MAX_PACKET_SIZE=256

; --- Primary target: LOLIN32 Lite v1.0.0 ---
[env:esp32]
platform = espressif32
board = lolin32_lite
upload_speed = 921600
; CH340 DTR/RTS auto-reset. If it fails, jumper GPIO0 → GND and tap RST.

; --- Fallback target: ESP8266 (higher deep-sleep current) ---
[env:esp8266]
platform = espressif8266
board = d1_mini
```

### config.h

```cpp
#pragma once
// ============================================================
//  Pool Temperature Monitor — Configuration
// ============================================================
//  Adjust the values below to match your setup, then build.
// ============================================================

// ----- Wi-Fi ------------------------------------------------
#define WIFI_SSID         "YOUR_WIFI_SSID"
#define WIFI_PASSWORD     "YOUR_WIFI_PASSWORD"

// ----- MQTT (Home Assistant) --------------------------------
#define MQTT_SERVER       "192.168.1.100"   // Home Assistant IP
#define MQTT_PORT         1883
#define MQTT_USER         "mqtt_user"        // leave "" if no auth
#define MQTT_PASSWORD     "mqtt_password"
#define MQTT_CLIENT_ID    "pool_temp"

// MQTT topics (Home Assistant auto-discovery uses these)
#define MQTT_TOPIC_TEMP   "homeassistant/sensor/pool_temp/state"
#define MQTT_TOPIC_BATT   "homeassistant/sensor/pool_battery/state"
#define MQTT_TOPIC_CONFIG_TEMP "homeassistant/sensor/pool_temp/config"
#define MQTT_TOPIC_CONFIG_BATT "homeassistant/sensor/pool_battery/config"

// ----- Sensor -----------------------------------------------
// DS18B20 data pin — use a GPIO that supports input on your board
#ifdef ESP32
  #define DS18B20_DATA_PIN   4     // GPIO4
  #define DS18B20_POWER_PIN  16    // GPIO16 — powers the sensor only when awake
#else  // ESP8266
  #define DS18B20_DATA_PIN   D4    // GPIO2
  #define DS18B20_POWER_PIN  D3    // GPIO0
#endif

// ----- Timing -----------------------------------------------
// Deep-sleep interval in microseconds (3 hours = 10,800,000,000 µs)
#define SLEEP_DURATION_US  (3ULL * 60ULL * 60ULL * 1000000ULL)

// Maximum seconds to wait for Wi-Fi before giving up and sleeping
#define WIFI_TIMEOUT_SEC   15

// Maximum MQTT connection retries
#define MQTT_MAX_RETRIES   3

// ----- Battery monitoring -----------------------------------
// 1 MΩ / 1 MΩ divider + 100 nF to GND at GPIO35.
#ifdef ESP32
  #define BATTERY_ADC_PIN    35    // GPIO35 (ADC1_CH7, input-only)
  #define ADC_RESOLUTION     4095.0
  #define ADC_REF_VOLTAGE    3.3
#else
  #define BATTERY_ADC_PIN    A0
  #define ADC_RESOLUTION     1023.0
  #define ADC_REF_VOLTAGE    1.0
#endif

#define VDIVIDER_RATIO     2.0
#define ADC_CAL_FACTOR     1.0    // set after DMM calibration
#define BATT_FULL_V        4.20
#define BATT_EMPTY_V       3.00
#define BATT_CUTOFF_V      3.20    // hibernate below this value

// ----- Static IP (optional) ---------------------------------
#define USE_STATIC_IP
#define STATIC_IP          192, 168, 1, 50
#define STATIC_GATEWAY     192, 168, 1, 1
#define STATIC_SUBNET      255, 255, 255, 0
#define STATIC_DNS1        192, 168, 1, 1
#define STATIC_DNS2        1, 1, 1, 1

// ----- On-board user LED ------------------------------------
#ifdef ESP32
  #define USER_LED_PIN       22
#endif
```

### main.cpp

Der vollständige Firmware-Quellcode liegt unter `firmware/src/main.cpp`. Kernpunkte:

- Brown-Out-Detector wird beim Booten abgeschaltet, um Resets durch kurze Spannungseinbrüche bei den ersten WLAN-TX-Bursts zu vermeiden.
- GPIO22 (User-LED) wird direkt am Anfang auf LOW gesetzt, damit die LED während der Aktivphase dunkel bleibt.
- Akkuspannung wird **vor** dem WLAN-Aufbau gemessen (nicht während der 200-mA-TX-Bursts).
- Liegt die Spannung unter `BATT_CUTOFF_V`, wird der Messzyklus übersprungen und das Gerät geht in Hibernation (kein Timer-Wake mehr — nur noch per RESET startbar), damit die Zelle nicht weiter entladen wird.
- Statische IP (`USE_STATIC_IP`) spart 2–4 Sekunden pro Zyklus gegenüber DHCP.
- ADC wird 32-fach gemittelt mit Settling-Delay, zusätzlich mit `ADC_CAL_FACTOR` kalibrierbar.
- Nach dem MQTT-Publish werden Sensor und WLAN abgeschaltet, dann Timer-Wake-Tiefschlaf für `SLEEP_DURATION_US` (Default 3 h).

---

## Home-Assistant-Einrichtung

### Voraussetzungen

- MQTT-Broker installiert (z. B. [Mosquitto-Add-on](https://github.com/home-assistant/addons/tree/master/mosquitto))
- MQTT-Integration in HA konfiguriert

### Auto-Discovery (empfohlen)

Die Firmware veröffentlicht MQTT-Discovery-Nachrichten. Wenn MQTT-Auto-Discovery aktiviert ist (Standard), erscheinen nach dem ersten Boot automatisch zwei Entitäten:

- `sensor.pool_temperature` — Wassertemperatur in °C
- `sensor.pool_monitor_battery` — Akkustand in %

Keine manuelle Konfiguration nötig.

### MQTT-Konfiguration (optional)

Nur nötig, wenn MQTT-Auto-Discovery deaktiviert ist. In die `configuration.yaml` einfügen:

```yaml
mqtt:
  sensor:
    - name: "Pool Temperature"
      unique_id: pool_temp_sensor
      state_topic: "homeassistant/sensor/pool_temp/state"
      unit_of_measurement: "°C"
      device_class: temperature
      value_template: "{{ value_json.temperature }}"

    - name: "Pool Monitor Battery"
      unique_id: pool_temp_battery
      state_topic: "homeassistant/sensor/pool_battery/state"
      unit_of_measurement: "%"
      device_class: battery
      value_template: "{{ value_json.battery }}"
```

### Automatisierungen

In die `automations.yaml` einfügen oder über die HA-Oberfläche anlegen:

```yaml
# --- Alarm bei Akku unter 20 % ---
- id: pool_monitor_low_battery
  alias: "Pool Monitor - Low Battery Alert"
  trigger:
    - platform: numeric_state
      entity_id: sensor.pool_monitor_battery
      below: 20
  condition:
    # Maximal einmal pro Tag auslösen
    - condition: template
      value_template: >
        {{ (as_timestamp(now()) - as_timestamp(
            state_attr('automation.pool_monitor_low_battery', 'last_triggered')
            | default(0))) > 86400 }}
  action:
    - service: persistent_notification.create
      data:
        title: "Pool Monitor Akku niedrig"
        message: >
          Pool-Temperaturmonitor ist auf
          {{ states('sensor.pool_monitor_battery') }} %.
          Bitte bald aufladen oder tauschen.
        notification_id: pool_battery_low
    # Optional: Mobile-Benachrichtigung
    # - service: notify.mobile_app_your_phone
    #   data:
    #     title: "Pool Monitor Akku niedrig"
    #     message: "Akku auf {{ states('sensor.pool_monitor_battery') }} %"

# --- Alarm bei ausgefallenem Sensor (keine Meldung über 7 h) ---
- id: pool_monitor_offline
  alias: "Pool Monitor - Offline Alert"
  trigger:
    - platform: state
      entity_id: sensor.pool_temperature
      to: "unavailable"
      for:
        hours: 7
  action:
    - service: persistent_notification.create
      data:
        title: "Pool Monitor Offline"
        message: >
          Der Pool-Temperatursensor hat seit über 7 Stunden nichts mehr gemeldet.
          Akku und WLAN-Signal prüfen.
        notification_id: pool_offline

# --- Hinweis bei idealer Badetemperatur ---
- id: pool_temp_ideal
  alias: "Pool - Temperature Ideal for Swimming"
  trigger:
    - platform: numeric_state
      entity_id: sensor.pool_temperature
      above: 24
  condition:
    - condition: time
      after: "08:00:00"
      before: "20:00:00"
  action:
    - service: persistent_notification.create
      data:
        title: "Pool bereit!"
        message: >
          Pool-Wassertemperatur beträgt {{ states('sensor.pool_temperature') }} °C
          — perfekt zum Schwimmen!
        notification_id: pool_ready
```

### Dashboard-Karten

Über "Dashboard bearbeiten" > "Karte hinzufügen" > "Manuell" einfügen:

**Option 1: Einfache Entities-Karte**

```yaml
type: entities
title: Pool Monitor
icon: mdi:pool-thermometer
entities:
  - entity: sensor.pool_temperature
    name: Wassertemperatur
    icon: mdi:thermometer-water
  - entity: sensor.pool_monitor_battery
    name: Monitor-Akku
    icon: mdi:battery
```

**Option 2: Reiche Karte mit Temperatur-Verlaufsgraph**

```yaml
type: vertical-stack
cards:
  - type: heading
    heading: Pool-Temperatur
    heading_style: title
    icon: mdi:pool-thermometer

  - type: horizontal-stack
    cards:
      - type: tile
        entity: sensor.pool_temperature
        name: Wassertemp.
        icon: mdi:thermometer-water
        color: blue

      - type: tile
        entity: sensor.pool_monitor_battery
        name: Akku
        icon: mdi:battery
        color: green

  - type: history-graph
    entities:
      - entity: sensor.pool_temperature
        name: Temperatur
    hours_to_show: 168   # 7 Tage
    title: Letzte 7 Tage
```

**Option 3: Gauge-Karte für schnellen Überblick**

```yaml
type: gauge
entity: sensor.pool_temperature
name: Pool-Temperatur
unit: "°C"
min: 0
max: 40
severity:
  green: 24
  yellow: 18
  red: 0
needle: true
```

---

## Gehäuse & Wasserdichtigkeit

### Anforderungen an das Gehäuse

- **Mindestens IP67** (steht in Wassernähe und im Freien)
- **ABS- oder Polycarbonat**-Abzweigdose, ~100 × 68 × 50 mm
- **PG7-Kabelverschraubung** für das DS18B20-Fühlerkabel
- **Silikon** rund um Verschraubung und Bohrungen

### Zusammenbau

1. Loch für die PG7-Kabelverschraubung in den Gehäuseboden bohren
2. Das DS18B20-Kabel durch die Verschraubung fädeln und festziehen
3. Rund um die Verschraubung Silikon aufbringen (zusätzlicher Schutz)
4. ESP32 und Akkuhalter mit Heißkleber oder Abstandshaltern im Inneren fixieren
5. Alles laut Schaltplan verdrahten
6. Gehäuse schließen — auf korrekten Sitz der Dichtung achten

### Montageoptionen

- **Am Poolrand**: Mit Kabelbindern oder Schrauben am Pool-Coping befestigen. Das Fühlerkabel ~30 cm tief ins Wasser hängen lassen.
- **Skimmer**: Im Skimmergehäuse platzieren. Der Fühler sitzt im Wasserstrom.
- **Schwimmend**: Gehäuse zusätzlich mit Silikon abdichten und schwimmen lassen. Ballast anbringen, damit der Fühler untergetaucht bleibt.

### Tipps zur Fühlerplatzierung

- Den Fühler mindestens **20 cm unter der Wasseroberfläche** anbringen, um sonnenaufgewärmte Oberflächenwerte zu vermeiden.
- Nicht in der Nähe von Einlaufdüsen platzieren (verfälschte, zu warme/kalte Werte).
- Der Edelstahlfühler ist chlorbeständig — keine Zusatzbeschichtung nötig.

---

## Optimierungstipps

### Akkulaufzeit mit dem LOLIN32 Lite weiter strecken

1. **Verpflichtend**: 3V3-Betriebs-LED auslöten. Das ist der größte Einzel-Gewinn — ~2 mA dauerhaft gespart.

2. **Statische IP-Adresse** — DHCP überspringen spart 2–4 Sekunden WLAN-Zeit pro Zyklus. Ist bereits verdrahtet; Werte setzen und `#define USE_STATIC_IP` in `config.h` beibehalten:
   ```cpp
   #define USE_STATIC_IP
   #define STATIC_IP       192, 168, 1, 50
   #define STATIC_GATEWAY  192, 168, 1, 1
   #define STATIC_SUBNET   255, 255, 255, 0
   ```

3. **Schlafintervall verlängern** — alle 6 Stunden statt alle 3 halbiert den Aktivverbrauch (für einen Pool völlig ausreichend).

4. **Brown-Out-Detektor bereits in der Firmware deaktiviert** — keine Aktion nötig.

5. **User-LED an GPIO22 bei Boot auf LOW gezogen** — bereits in der Firmware. Wenn deine Charge trotzdem ein schwaches Glimmen zeigt, in `main.cpp` das `digitalWrite(USER_LED_PIN, LOW)` auf `HIGH` ändern.

6. **Fortgeschritten: onboard-ME6211 umgehen**, indem man seine VIN-Leiterbahn auftrennt und einen externen HT7333 in den 3V3-Header einspeist. Reduziert den Ruhestrom von ~50 µA auf ~4 µA. Verlängert die Laufzeit von ~18 auf ~30 Monate. Nur sinnvoll bei Mehrjahreszielen.

7. **ESP-NOW statt MQTT** — wenn ein zweiter, dauerhaft netzbetriebener ESP32 als Gateway da ist, verbindet ESP-NOW in ~10 ms statt ~3 s für WLAN+MQTT. Reduziert die Aktivzeit um ~100×.

### Zu den 1,5-V-USB-C-AA-Akkus

Diese USB-C-aufladbaren AA-Zellen geben über einen integrierten Boost-Konverter konstant 1,5 V aus. Sie funktionieren hier **nicht**, weil:
- Der ESP32 mindestens 3,3 V braucht; 1,5 V aus einer Zelle reichen selbst über einen Boost nicht aus.
- Zwei in Serie (3,0 V) liegen immer noch unter 3,3 V, und die internen Boost-Schaltungen arbeiten gegeneinander.
- Der eingebaute Regler verschwendet Energie im Tiefschlaf.

Beim NCR18650B + TP4054-Lader des LOLIN32 Lite bleiben — das ist der standardmäßige, gut charakterisierte Weg.

---

## Fehlersuche

| Symptom | Ursache | Lösung |
|---|---|---|
| Keine WLAN-Verbindung | Falsche Zugangsdaten oder schwaches Signal | `config.h` prüfen. Sicherstellen, dass der AP 2,4 GHz sendet (der ESP32 kann kein 5 GHz). Externe Antenne auf diesem Board nicht möglich — bei RSSI < −75 dBm Gerät oder AP umstellen. |
| Temperatur meldet −127 °C | DS18B20-Verdrahtungsfehler | GPIO4 (DATA), 4,7-kΩ-Pull-up zwischen DATA und 3V3 und GPIO16 (VDD) prüfen. |
| Akku in Wochen leer | 3V3-Betriebs-LED leuchtet noch | Grüne Betriebs-LED neben dem 3V3-Pin-Header physisch auslöten. Mit Multimeter bestätigen: Tiefschlaf-Strom auf der JST-Leitung muss < 120 µA sein. |
| User-LED an GPIO22 glimmt leicht während Wachphase | Active-Low-Verdrahtung der LED in deiner Charge | In `main.cpp` `setup()` das `digitalWrite(USER_LED_PIN, LOW)` auf `HIGH` ändern. |
| Sensor erscheint nicht in HA | MQTT falsch konfiguriert | Broker-IP, Zugangsdaten prüfen. Mit `mosquitto_sub -t '#'` verifizieren, ob Nachrichten ankommen. |
| Flashen schlägt fehl, "Connecting…" Timeout | CH340-Auto-Reset löst nicht aus | GPIO0 auf GND jumpern, den onboard-RST kurz drücken, GPIO0 freigeben, erneut flashen. Aktuellsten CH340-Treiber am Host installieren. |
| Ungenaue Akku-Prozente | ADC-Nichtlinearität | Reale Zellspannung mit einem DMM messen, mit dem gemeldeten Wert vergleichen, in `config.h` `ADC_CAL_FACTOR` auf (DMM ÷ gemeldet) setzen. |
| Laden startet nicht | Zellspannung zu niedrig (< 2,5 V) oder Schutzbeschaltung hat ausgelöst | Zelle entnehmen, kurz (~10 s) an eine 4,2-V-Quelle halten, um das PCM zu entriegeln, wieder einsetzen. Lädt sie dann immer noch nicht, ist die Zelle am Lebensende. |
| Werte driften mit der Zeit | Fühler verschmutzt oder korrodiert | Fühler reinigen. Edelstahl-DS18B20-Fühler sind chlorbeständig, aber jährlich inspizieren. |

---

## Projekt-Dateistruktur

```
firmware/
├── platformio.ini                 # PlatformIO-Buildkonfiguration
├── include/
│   └── config.h                   # WLAN-, MQTT-, Pin-, Timing-Einstellungen
└── src/
    └── main.cpp                   # Haupt-Firmware (wake → read → publish → sleep)

homeassistant/
├── configuration.yaml             # Optionale manuelle MQTT-Sensor-Konfiguration
├── automations.yaml               # Akku-, Offline- und Badetemperatur-Alarme
└── dashboard-card.yaml            # Lovelace-Dashboard-Karten (3 Varianten)
```

---

## Lizenz

Dieses Projekt wird "as-is" für den persönlichen / edukativen Gebrauch bereitgestellt. Keine ausdrückliche oder stillschweigende Garantie.
