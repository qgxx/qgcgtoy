var cameraPosition = [30, 30, 30]

var resolution = 2048;
var fbo;

GAMES202Main();

function GAMES202Main() {
	// Init canvas and gl
	const canvas = document.querySelector('#glcanvas');
	canvas.width = window.screen.width;
	canvas.height = window.screen.height;
	const gl = canvas.getContext('webgl');
	if (!gl) {
		alert('Unable to initialize WebGL. Your browser or machine may not support it.');
		return;
	}

	// Add camera
	const camera = new THREE.PerspectiveCamera(75, gl.canvas.clientWidth / gl.canvas.clientHeight, 1e-2, 1000);
	camera.position.set(cameraPosition[0], cameraPosition[1], cameraPosition[2]);

	// Add resize listener
	function setSize(width, height) {
		camera.aspect = width / height;
		camera.updateProjectionMatrix();
	}
	setSize(canvas.clientWidth, canvas.clientHeight);
	window.addEventListener('resize', () => setSize(canvas.clientWidth, canvas.clientHeight));

	// Add camera control
	const cameraControls = new THREE.OrbitControls(camera, canvas);
	cameraControls.enableZoom = true;
	cameraControls.enableRotate = true;
	cameraControls.enablePan = true;
	cameraControls.rotateSpeed = 0.3;
	cameraControls.zoomSpeed = 1.0;
	cameraControls.panSpeed = 0.8;
	cameraControls.target.set(0, 0, 0);

	// Add renderer
	const renderer = new WebGLRenderer(gl, camera);

	
	// Add lights
	// light - is open shadow map == true
	let lightPos1 = [0, 80, 80];
	let focalPoint = [0, 0, 0];
	let lightUp = [0, 1, 0]
	const directionLight = new DirectionalLight(2500, [1, 1, 1], lightPos1, focalPoint, lightUp, true, renderer.gl);
	renderer.addLight(directionLight);

	let lightPos2 = [90, 90, 0];
	const directionLight2 = new DirectionalLight(2500, [1, 1, 1], lightPos2, focalPoint, lightUp, true, renderer.gl);
	renderer.addLight(directionLight2);

	// Add shapes
	let floorTransform = setTransform(0, 0, -30, 0, 0, 0, 4, 4, 4);
	let obj1Transform = setTransform(0, 0, 0, 0, 0, 0, 20, 20, 20);
	let obj2Transform = setTransform(40, 0, -40, 0, 0, 0, 10, 10, 10);

	loadOBJ(renderer, '../../../assets/models/mary/', 'Marry', 'PhongMaterial', obj1Transform, 'Marry1');
	loadOBJ(renderer, '../../../assets/models/mary/', 'Marry', 'PhongMaterial', obj2Transform, 'Marry2');
	loadOBJ(renderer, '../../../assets/models/floor/', 'floor', 'PhongMaterial', floorTransform);

	var guiParams = {
		modelTransX: 0,
		modelTransY: 0,
		modelTransZ: 0,
		shadowClass: 2
	}

	const shadowModeSettings = {
		mode: 'SM'
	}

	const shadowModeMap = new Map([
		['SM', 0],
		['PCF', 1],
		['PCSS', 2]
	]);

	function createGUI() {
		const gui = new dat.gui.GUI();
		const panelModel = gui.addFolder('Model properties');
		const panelModelTrans = panelModel.addFolder('Translation');
		panelModelTrans.add(guiParams, 'modelTransX').name('X');
		panelModelTrans.add(guiParams, 'modelTransY').name('Y');
		panelModelTrans.add(guiParams, 'modelTransZ').name('Z');
		const shadowClass = gui.addFolder('Shadow Class');
		shadowClass.add(shadowModeSettings, 'mode', ['SM', 'PCF', 'PCSS']).name('Select Mode').onChange(function(value){
			guiParams.shadowClass = shadowModeMap.get(value);
		});
		panelModel.open();
		panelModelTrans.open();
		shadowClass.open();
	}
	createGUI();

	let prevTime = 0;

	function mainLoop(now) {
		cameraControls.update();
		let deltaime = (now - prevTime) / 1000;
		renderer.render(now, deltaime, guiParams);
		requestAnimationFrame(mainLoop);
		prevTime = now;
	}
	requestAnimationFrame(mainLoop);
}

function setTransform(t_x, t_y, t_z, r_x, r_y, r_z, s_x, s_y, s_z) {
	return {
		modelTransX: t_x,
		modelTransY: t_y,
		modelTransZ: t_z,
		modelRotateX: r_x,
		modelRotateY: r_y,
		modelRotateZ: r_z,
		modelScaleX: s_x,
		modelScaleY: s_y,
		modelScaleZ: s_z,
	};
}

function degrees2Radians(degrees){
	return 3.1415927 / 180 * degrees;
}
