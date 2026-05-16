import { supabase } from './supabaseClient.js'

const stationBox = document.getElementById('stationBox')
const latestMeasurementBox = document.getElementById('latestMeasurement')
const measurementsBox = document.getElementById('measurements')

let weatherChart = null

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

    if (error) {
        measurementsBox.innerHTML = `<p class="error">Measurements hiba: ${error.message}</p>`
        return
    }

    if (!data || data.length === 0) {
        measurementsBox.innerHTML = '<p>Nincs mérési adat.</p>'
        if (latestMeasurementBox) {
            latestMeasurementBox.innerHTML = '<p>Nincs aktuális mérés.</p>'
        }
        return
    }

    const latest = data[0]

    if (latestMeasurementBox) {
        latestMeasurementBox.innerHTML = `
            <div class="latest-card">
                <div class="temperature-big">
                <span class="label">Hőmérséklet: </span>
                    ${latest.temperature_c_x100 / 100} °C
                </div>

                <div class="latest-grid">
                    <div class="latest-item">
                        <span class="label">Páratartalom:</span>
                        <span class="value">${latest.humidity_x100 / 100} %</span>
                    </div>

                    <div class="latest-item">
                        <span class="label">Nyomás:</span>
                        <span class="value">${latest.pressure_hpa_x100 / 100} hPa</span>
                    </div>

                    <div class="latest-item">
                        <span class="label">Fény:</span>
                        <span class="value">${latest.lux ?? 'nincs adat'} lux</span>
                    </div>

                    <div class="latest-item">
                        <span class="label">Utolsó frissítés:</span>
                        <span class="value">
                            ${latest.measured_at
                                ? new Date(latest.measured_at).toLocaleString('hu-HU')
                                : 'nincs adat'}
                        </span>
                    </div>
                </div>
            </div>
        `
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

async function loadChartData() {
    const { data, error } = await supabase
        .from('measurements')
        .select('temperature_c_x100, humidity_x100, measured_at, id')
        .order('id', { ascending: false })
        .limit(30)

    if (error) {
        console.log('Chart hiba:', error)
        return
    }

    if (!data || data.length === 0) {
        console.log('Nincs grafikon adat.')
        return
    }

    const reversedData = data.reverse()

    const labels = reversedData.map(m => {
        if (m.measured_at) {
            return new Date(m.measured_at).toLocaleTimeString('hu-HU', {
                hour: '2-digit',
                minute: '2-digit'
            })
        }

        return `#${m.id}`
    })

    const temperatures = reversedData.map(m => m.temperature_c_x100 / 100)
    const humidities = reversedData.map(m => m.humidity_x100 / 100)

    const ctx = document.getElementById('weatherChart')

    if (!ctx) {
        console.log('Hiányzik a weatherChart canvas a HTML-ből.')
        return
    }

    if (weatherChart !== null) {
        weatherChart.destroy()
    }

    weatherChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels,
            datasets: [
               
                {
                    label: 'Páratartalom (%)',
                    data: humidities,
                    borderWidth: 2,
                    tension: 0.3
                },
                 {
                    label: 'Hőmérséklet (°C)',
                    data: temperatures,
                    borderWidth: 2,
                    tension: 0.3
                },
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true
                }
            },
            scales: {
                y: {
                    beginAtZero: false
                }
            }
        }
    })
}

loadStations()
loadMeasurements()
loadChartData()

const measurementsChannel = supabase
    .channel('measurements-realtime-channel')
    .on(
        'postgres_changes',
        {
            event: 'INSERT',
            schema: 'public',
            table: 'measurements'
        },
        () => {
            loadMeasurements()
            loadChartData()
        }
    )
    .subscribe((status) => {
        console.log('Realtime státusz:', status)
    })