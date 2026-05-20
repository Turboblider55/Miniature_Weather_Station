import { supabase } from './supabaseClient.js'

const stationBox = document.getElementById('stationBox')
const latestMeasurementBox = document.getElementById('latestMeasurement')
const measurementsBox = document.getElementById('measurements')
const chartModeSelect = document.getElementById('chartMode')

let weatherChart = null

const chartFields = [
    {
        checkboxId: 'chartTemperature',
        label: 'Hőmérséklet (°C)',
        field: 'temperature_c_x100',
        convert: value => value / 100
    },
    {
        checkboxId: 'chartHumidity',
        label: 'Páratartalom (%)',
        field: 'humidity_x100',
        convert: value => value / 100
    },
    {
        checkboxId: 'chartPressure',
        label: 'Nyomás (hPa)',
        field: 'pressure_hpa_x100',
        convert: value => value / 100
    },
    {
        checkboxId: 'chartAqi',
        label: 'AQI',
        field: 'aqi',
        convert: value => value
    },
    {
        checkboxId: 'chartTvoc',
        label: 'TVOC (ppb)',
        field: 'tvoc_ppb',
        convert: value => value
    },
    {
        checkboxId: 'chartEco2',
        label: 'eCO2 (ppm)',
        field: 'eco2_ppm',
        convert: value => value
    },
    {
        checkboxId: 'chartLux',
        label: 'Fény (lux)',
        field: 'lux',
        convert: value => value
    },
    {
        checkboxId: 'chartCloud',
        label: 'Felhő index',
        field: 'cloud_index',
        convert: value => value
    },
    {
        checkboxId: 'chartAltitude',
        label: 'Magasság (m)',
        field: 'altitude_m_x10',
        convert: value => value / 10
    }
]

function getSelectedChartFields() {
    return chartFields.filter(item => {
        const checkbox = document.getElementById(item.checkboxId)
        return checkbox && checkbox.checked
    })
}

function setupChartCheckboxes() {
    chartFields.forEach(item => {
        const checkbox = document.getElementById(item.checkboxId)

        if (checkbox) {
            checkbox.addEventListener('change', () => {
                loadChartData()
            })
        }
    })
}

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
            <h3>${station.name}</h3>
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
            <p><strong>Dátum:</strong> ${
                m.measured_at
                    ? new Date(m.measured_at).toLocaleString('hu-HU')
                    : 'nincs adat'
            }</p>
        </div>
    `).join('')
}

async function loadChartOptions() {
    if (!chartModeSelect) return

    const { data, error } = await supabase
        .from('measurements')
        .select('measured_at')
        .not('measured_at', 'is', null)
        .order('measured_at', { ascending: false })

    if (error) {
        console.log('Dátum lista hiba:', error)
        return
    }

    const selectedValue = chartModeSelect.value

    const uniqueDates = [...new Set(data.map(m => {
        const date = new Date(m.measured_at)
        const year = date.getFullYear()
        const month = String(date.getMonth() + 1).padStart(2, '0')
        const day = String(date.getDate()).padStart(2, '0')

        return `${year}-${month}-${day}`
    }))]

    chartModeSelect.innerHTML = '<option value="latest30">Elmúlt 30 mérés</option>'

    uniqueDates.forEach(date => {
        const option = document.createElement('option')
        option.value = date
        option.textContent = date
        chartModeSelect.appendChild(option)
    })

    if ([...chartModeSelect.options].some(option => option.value === selectedValue)) {
        chartModeSelect.value = selectedValue
    }
}

async function loadChartData() {
    let query = supabase
        .from('measurements')
        .select('*')

    if (!chartModeSelect || chartModeSelect.value === 'latest30') {
        query = query
            .order('id', { ascending: false })
            .limit(30)
    } else {
        const selectedDate = chartModeSelect.value
        const startDate = new Date(`${selectedDate}T00:00:00`)
        const endDate = new Date(`${selectedDate}T23:59:59`)

        query = query
            .gte('measured_at', startDate.toISOString())
            .lte('measured_at', endDate.toISOString())
            .order('measured_at', { ascending: true })
    }

    const { data, error } = await query

    if (error) {
        console.log('Chart hiba:', error)
        return
    }

    if (!data || data.length === 0) {
        console.log('Nincs grafikon adat.')

        if (weatherChart !== null) {
            weatherChart.destroy()
            weatherChart = null
        }

        return
    }

    let chartData = data

    if (!chartModeSelect || chartModeSelect.value === 'latest30') {
        chartData = data.reverse()
    }

    const selectedFields = getSelectedChartFields()

    if (selectedFields.length === 0) {
        if (weatherChart !== null) {
            weatherChart.destroy()
            weatherChart = null
        }

        console.log('Nincs kiválasztott adat a grafikonhoz.')
        return
    }

    const labels = chartData.map(m => {
        if (m.measured_at) {
            const date = new Date(m.measured_at)

            const day = date.toLocaleDateString('hu-HU', {
                month: '2-digit',
                day: '2-digit'
            })

            const time = date.toLocaleTimeString('hu-HU', {
                hour: '2-digit',
                minute: '2-digit'
            })

            return [day, time]
        }

        return [`#${m.id}`]
    })

const datasets = selectedFields.map(item => {

    let borderColor = '#64748b'

    switch (item.field) {

        case 'temperature_c_x100':
            borderColor = '#ef4444'
            break

        case 'humidity_x100':
            borderColor = '#3b82f6'
            break

        case 'pressure_hpa_x100':
            borderColor = '#10b981'
            break

        case 'aqi':
            borderColor = '#f59e0b'
            break

        case 'tvoc_ppb':
            borderColor = '#8b5cf6'
            break

        case 'eco2_ppm':
            borderColor = '#ec4899'
            break

        case 'lux':
            borderColor = '#eab308'
            break

        case 'cloud_index':
            borderColor = '#06b6d4'
            break

        case 'altitude_m_x10':
            borderColor = '#84cc16'
            break
    }

    return {
        label: item.label,

        data: chartData.map(m => {
            const value = m[item.field]

            if (value === null || value === undefined || value === -1) {
                return null
            }

            return item.convert(value)
        }),

        borderWidth: 2,
        tension: 0.3,

        borderColor: borderColor,
        backgroundColor: borderColor
    }
})

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
            datasets: datasets
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
loadChartOptions()
loadChartData()
setupChartCheckboxes()

if (chartModeSelect) {
    chartModeSelect.addEventListener('change', () => {
        loadChartData()
    })
}

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
            loadChartOptions()
            loadChartData()
        }
    )
    .subscribe((status) => {
        console.log('Realtime státusz:', status)
    })