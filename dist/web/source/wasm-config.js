let wasmSize = null;
// See comment in dist/web/Dockerfile about fenestra.wasm.size
fetch("fenestra.wasm.size").then(async (resp) => {
    wasmSize = parseInt((await resp.text()).trim());
    console.log(`Real WASM binary size is ${wasmSize} bytes`);
});

// Monkeypatch WebAssembly to have a progress bar
// inspired from: https://github.com/WordPress/wordpress-playground/pull/46 (but had to be modified)
function monkeyPatch(progressFun) {
    const _instantiateStreaming = WebAssembly.instantiateStreaming;
    WebAssembly.instantiateStreaming = (response, ...args) => {
        // Do not collect wasm content length here see above
        const file = response.url.substring(
            new URL(response.url).origin.length + 1
        );
        const reportingResponse = new Response(
            new ReadableStream(
                {
                    async start(controller) {
                        const reader = response.clone().body.getReader();
                        let loaded = 0;
                        for (; ;) {
                            const { done, value } = await reader.read();
                            if (done) {
                                if(wasmSize) progressFun(file, wasmSize);
                                break;
                            }
                            loaded += value.byteLength;
                            progressFun(file, loaded);
                            controller.enqueue(value);
                        }
                        controller.close();
                    }
                },
                {
                    status: response.status,
                    statusText: response.statusText
                }
            )
        );
        for (const pair of response.headers.entries()) {
            reportingResponse.headers.set(pair[0], pair[1]);
        }

        return _instantiateStreaming(reportingResponse, ...args);
    }
}
monkeyPatch((file, done) =>  {
    if (!wasmSize) return;
    if (done > wasmSize) {
        console.warn(`Downloaded binary size ${done} is larger than expected WASM size ${wasmSize}`);
        return;
    }

    const percent  = ((done / wasmSize) * 100).toFixed(0);
    const mibNow   = (done / 1024**2).toFixed(1);
    const mibTotal = (wasmSize / 1024**2).toFixed(1);

    let root = document.querySelector(':root');
    root.style.setProperty("--progress", `${percent}%`)
    document.getElementById("progress-bar-content").innerHTML = `${percent}% &nbsp;[${mibNow} MiB / ${mibTotal} MiB]`;
});

var notWorkingTimer = setTimeout(() => {
    document.getElementById("not_working").classList.add("visible")
}, 5000);

var Module = {
    preRun:  [],
    postRun: function() {
    },
    onRuntimeInitialized: function() {
        // Triggered when the wasm module is loaded and ready to use.
        document.getElementById("loading").style.display = "none"
        document.getElementById("canvas").style.display = "initial"

        clearTimeout(notWorkingTimer);
    },
    print:   (function() { })(),
    printErr: function(text) {  },
    canvas: (function() {
        const canvas = document.getElementById('canvas');
        canvas.addEventListener("webglcontextlost", function(e) {
            alert('WebGL context lost, please reload the page');
            e.preventDefault();
        }, false);

        // Turn long touches into right-clicks
        let timer = null;
        canvas.addEventListener('touchstart', event => {
            timer = setTimeout(() => {
                let eventArgs = {
                    bubbles: true,
                    cancelable: true,
                    view: window,
                    screenX: event.touches[0].screenX,
                    screenY: event.touches[0].screenY,
                    clientX: event.touches[0].clientX,
                    clientY: event.touches[0].clientY,
                    button: 2,
                    buttons: 2,
                    relatedTarget: event.target,
                    region: event.region
                }

                canvas.dispatchEvent(new MouseEvent('mousedown', eventArgs));
                canvas.dispatchEvent(new MouseEvent('mouseup', eventArgs));
            }, 400);
        });

        canvas.addEventListener('touchend', event => {
            if (timer) {
                clearTimeout(timer);
                timer = null;
            }
        });

        if (typeof WebGL2RenderingContext !== 'undefined') {
            let gl = canvas.getContext('webgl2', { stencil: true });
            if (!gl) {
                console.error('WebGL 2 not available, falling back to WebGL');
                gl = canvas.getContext('webgl', { stencil: true });
            }
            if (!gl) {
                alert('WebGL not available with stencil buffer');
            }
            return canvas;
        } else {
            alert('WebGL 2 not supported by this browser');
        }
    })(),
    setStatus: function(text) { },
    totalDependencies: 0,
    monitorRunDependencies: function(left) {
    },
    instantiateWasm: function(imports, successCallback) {
        instantiateAsync(wasmBinary, wasmBinaryFile, imports, (result) => {
            successCallback(result.instance, result.module)
        });
    },
    arguments: []
};

// Handle passing arguments to the wasm module
const queryString = window.location.search;
const urlParams = new URLSearchParams(queryString);
if (urlParams.has("lang")) {
    Module["arguments"].push("--language");
    Module["arguments"].push(urlParams.get("lang"));
}

window.addEventListener('resize', js_resizeCanvas, false);
function js_resizeCanvas() {
    let canvas = document.getElementById('canvas');

    canvas.top    = document.documentElement.clientTop;
    canvas.left   = document.documentElement.clientLeft;
    canvas.width  = Math.min(document.documentElement.clientWidth, window.innerWidth || 0);
    canvas.height = Math.min(document.documentElement.clientHeight, window.innerHeight || 0);
}