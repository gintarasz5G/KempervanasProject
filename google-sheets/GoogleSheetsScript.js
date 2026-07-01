// =====================================================
//   GET: paskutinė eilutė kaip JSON (?action=latest)
// =====================================================
function doGet(e) {
  var action = (e && e.parameter && e.parameter.action) ? e.parameter.action : '';
  if (action !== 'latest') {
    return ContentService.createTextOutput('Kemperis Sheets API veikia')
      .setMimeType(ContentService.MimeType.TEXT);
  }
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var tz = ss.getSpreadsheetTimeZone(); // Automatiškai prisitaiko prie vasaros/žiemos laiko
  var now = new Date();
  var sheet = null;
  // Bandome dabartinį mėnesį, tada praėjusį
  for (var attempt = 0; attempt <= 1; attempt++) {
    var d = new Date(now.getFullYear(), now.getMonth() - attempt, 1);
    var monthName = Utilities.formatDate(d, tz, "yyyy-MM");
    var s = ss.getSheetByName(monthName);
    if (s && s.getLastRow() > 1) { sheet = s; break; }
  }
  if (!sheet) {
    return ContentService.createTextOutput(JSON.stringify({ error: 'No data' }))
      .setMimeType(ContentService.MimeType.JSON);
  }
  var lastRow = sheet.getLastRow();
  var numCols = sheet.getLastColumn();
  var vals = sheet.getRange(lastRow, 1, 1, numCols).getValues()[0];
  // Stulpelių žemėlapis:
  // 0:ts, 1:GPS (rašo skriptas), 2:lat, 3:lon, 4:alt, 5:speed, 6:heading,
  // 7:soc, 8:voltage, 9:current, 10:water_pct, 11:tds, 12:gas_pct,
  // 13:bme_temp, 14:bme_hum, 15:pressure, 16:co2,
  // 17:bedroom_temp, 18:bedroom_hum, 19:power_w, 20:ah_remaining, 21:uptime_min
  // 22: received_timestamp (doPost prideda)
  var result = {
    timestamp:        vals[0] ? Utilities.formatDate(new Date(vals[0]), tz, "yyyy-MM-dd HH:mm:ss") : '',
    lat:              parseFloat(vals[2])  || 0,
    lon:              parseFloat(vals[3])  || 0,
    soc:              parseFloat(vals[7])  || 0,
    voltage:          parseFloat(vals[8])  || 0,
    current:          parseFloat(vals[9])  || 0,
    water_pct:        parseFloat(vals[10]) || 0,
    tds_ppm:          parseFloat(vals[11]) || 0,
    gas_pct:          parseFloat(vals[12]) || 0,
    bme_temp:         parseFloat(vals[13]) || 0,
    bme_humidity:     parseFloat(vals[14]) || 0,
    bme_pressure:     parseFloat(vals[15]) || 0,
    co2:              parseFloat(vals[16]) || 0,
    bedroom_temp:     parseFloat(vals[17]) || 0,
    bedroom_humidity: parseFloat(vals[18]) || 0,
    power_w:          parseFloat(vals[19]) || 0,
    ah_remaining:     parseFloat(vals[20]) || 0,
    uptime_min:       parseFloat(vals[21]) || 0,
    received:         String(vals[22] || '')
  };
  return ContentService.createTextOutput(JSON.stringify(result))
    .setMimeType(ContentService.MimeType.JSON);
}

// =====================================================
//   POST: duomenų priėmimas ir įrašymas (Batch)
// =====================================================
function doPost(e) {
  if (!e || !e.postData || !e.postData.contents) {
    return ContentService.createTextOutput("Nėra duomenų");
  }
  var batchData = e.postData.contents.trim();
  if (batchData.length === 0) return ContentService.createTextOutput("Tuščia");

  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var tz = ss.getSpreadsheetTimeZone();
  var now = new Date();
  var timestamp = Utilities.formatDate(now, tz, "yyyy-MM-dd HH:mm:ss");

  // Nustatome esamo mėnesio lapo pavadinimą (pvz., "2026-06")
  var sheetName = Utilities.formatDate(now, tz, "yyyy-MM");

  var rows = batchData.split(';');
  var allRows = [];
  var maxCols = 0;

  for (var i = 0; i < rows.length; i++) {
    var row = rows[i].trim();
    if (row.length < 5) continue; // tuščios arba per trumpos – praleisti
    try {
      var cols = row.split(',');
      // GPS nuoroda į 2-ąjį stulpelį (indeksas 1)
      if (cols.length >= 4) {
        var lat = cols[2];
        var lon = cols[3];
        if (lat !== "0.000000" && lon !== "0.000000" &&
            lat !== "0" && lon !== "0" && lat !== "") {
          cols[1] = '=HYPERLINK("https://www.google.com/maps?q=' + lat + ',' + lon + '", "📌 Žemėlapis")';
        } else {
          cols[1] = "Nėra GPS signalo";
        }
      }
      // Gavimo laikas – paskutinis stulpelis
      cols.push(timestamp);
      if (cols.length > maxCols) maxCols = cols.length;
      allRows.push(cols);
    } catch (err) {
      Logger.log("Klaida apdorojant eilutę [" + i + "]: " + row);
    }
  }

  if (allRows.length === 0) return ContentService.createTextOutput("Visos eilutės atmestos");

  // Normalizuojame: visos eilutės privalo turėti lygiai maxCols stulpelių
  // (setValues() reikalauja stačiakampio masyvo)
  for (var j = 0; j < allRows.length; j++) {
    while (allRows[j].length < maxCols) allRows[j].push("");
  }

  // Lapo paieška arba sukūrimas — rašo griežtai į šio mėnesio lapą
  var sheet = ss.getSheetByName(sheetName);
  if (!sheet) {
    sheet = ss.insertSheet(sheetName);
  }

  // Vienas batch write – vietoj N appendRow() užklausų
  var lastRow = sheet.getLastRow();
  sheet.getRange(lastRow + 1, 1, allRows.length, maxCols).setValues(allRows);

  return ContentService.createTextOutput("200 OK: " + allRows.length + " eilutės įrašytos");
}
