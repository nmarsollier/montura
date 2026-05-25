function mountApp() {
    return {
        status: {ra: '--:--:--', dec: '--:--:--'},
        statusText: 'UNKNOWN',
        settings: {lat: 0, lon: 0, elevation: 0},
        speed: 1,
        speedOptions: [1, 2, 3, 4],
        degrees: 5,
        wifi: {ssid: '', password: ''},
        // Current tracking mode reported by the device.
        serverTracking: 'none',
        wifiAp: false,
        // Selected tracking mode index for the control.
        selectedTracking: null,
        trackingOptions: ['none', 'sidereal', 'lunar', 'solar'],

        fetchStatus() {
            fetch('/api/status').then(r => r.json()).then(j => {
                this.statusText = j.status || 'UNKNOWN';
                this.status = {ra: j.ra || '--:--:--', dec: j.dec || '--:--:--'};
                this.settings = j.settings || {lat: 0, lon: 0, elevation: 0};
                this.serverTracking = j.tracking || 'none';
                this.wifiAp = j.wifi_ap || false;
                // Initialize the select once so polling does not overwrite user input.
                if (this.selectedTracking === null) {
                    const idx = this.trackingOptions.indexOf(j.tracking);
                    this.selectedTracking = idx >= 0 ? idx : 0;
                }
            }).catch(() => {
            });
        },

        stop() {
            fetch('/api/stop', {method: 'POST'}).then(() => this.fetchStatus());
        },
        home() {
            fetch('/api/home', {method: 'POST'}).then(() => this.fetchStatus());
        },
        park() {
            fetch('/api/park', {method: 'POST'}).then(() => this.fetchStatus());
        },

        setTracking() {
            fetch('/api/tracking', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({tracking: this.trackingOptions[this.selectedTracking]})
            }).then(() => this.fetchStatus());
        },

        slew(axis, dir) {
            const axisName = axis === 'ra' ? 'ra' : 'dec';
            const deg = Math.min(90, Math.max(1, Math.round(Math.abs(this.degrees))));
            fetch('/api/move-axis', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({axis: axisName, degrees: dir * deg, speed: this.speed})
            }).then(() => this.fetchStatus());
        },

        init() {
            this.fetchStatus();
            setInterval(() => this.fetchStatus(), 1000);
        }
    }
}

window.addEventListener('load', () => {
    setTimeout(() => {
        const root = document.querySelector('[x-data]');
        if (root && root.__x) root.__x.$data.init();
    }, 500);
});
