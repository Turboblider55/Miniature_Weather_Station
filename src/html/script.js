import { supabase } from './supabaseClient.js'

const stationBox = document.getElementById('stationBox')
const measurementsBox = document.getElementById('measurements')

async function loadStations() {
    const { data, error } = await supabase
        .from('stations')
        .select('*')

    if (error) {
        stationBox.innerHTML = `<p class="error">Stations hiba: ${error.message}</p>`
        return
    }

    stationBox.innerHTML = data.map(station => `
        <div class="card">
            <h2>${station.name}</h2>
            <p>Állapot: ${station.online ? 'Online' : 'Offline'}</p>
        </div>
    `).join('')
}

async function loadMeasurements() {
    measurementsBox.innerHTML = '<p>Mérések betöltése...</p>'

    const { data, error } = await supabase
        .from('measurements')
        .select('*')
        .order('id', { ascending: false })
        .limit(10)

    console.log('Measurements:', data)
    console.log('Error:', error)

    if (error) {
        measurementsBox.innerHTML = `<p class="error">Measurements hiba: ${error.message}</p>`
        return
    }

    if (!data || data.length === 0) {
        measurementsBox.innerHTML = '<p>Nincs mérési adat.</p>'
        return
    }

    measurementsBox.innerHTML = data.map(m => `
        <div class="card">
            <h3>Mérés #${m.id}</h3>
            <p><strong>Hőmérséklet:</strong> ${m.temperature_c_x100 / 100} °C</p>
            <p><strong>Páratartalom:</strong> ${m.humidity_x100 / 100} %</p>
            <p><strong>Nyomás:</strong> ${m.pressure_hpa_x100 / 100} hPa</p>
            <p><strong>AQI:</strong> ${m.aqi ?? 'nincs adat'}</p>
            <p><strong>TVOC:</strong> ${m.tvoc_ppb} ppb</p>
            <p><strong>eCO2:</strong> ${m.eco2_ppm} ppm</p>
            <p><strong>Magasság:</strong> ${m.altitude_m_x10 / 10} m</p>
            <p><strong>Fény:</strong> ${m.lux ?? 'nincs adat'} lux</p>
            <p><strong>Felhő index:</strong> ${m.cloud_index ?? 'nincs adat'}</p>
        </div>
    `).join('')
}

loadStations()
loadMeasurements()
setInterval(loadMeasurements, 10000)