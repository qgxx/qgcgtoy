function loadOBJ(renderer, path, name, objMaterial, transform, objName) {

	const manager = new THREE.LoadingManager();
	manager.onProgress = function (item, loaded, total) {
		console.log(item, loaded, total);
	};

	function onProgress(xhr) {
		if (xhr.lengthComputable) {
			const percentComplete = xhr.loaded / xhr.total * 100;
			console.log('model ' + Math.round(percentComplete, 2) + '% downloaded');
		}
	}
	function onError() { }

	new THREE.MTLLoader(manager)
		.setPath(path)
		.load(name + '.mtl', function (materials) {
			materials.preload();
			new THREE.OBJLoader(manager)
				.setMaterials(materials)
				.setPath(path)
				.load(name + '.obj', function (object) {
					object.traverse(function (child) {
						if (child.isMesh) {
							let geo = child.geometry;
							let mat;
							if (Array.isArray(child.material)) mat = child.material[0];
							else mat = child.material;

							var indices = Array.from({ length: geo.attributes.position.count }, (v, k) => k);
							let mesh = new Mesh({ name: 'aVertexPosition', array: geo.attributes.position.array },
								{ name: 'aNormalPosition', array: geo.attributes.normal.array },
								{ name: 'aTextureCoord', array: geo.attributes.uv.array },
								indices, transform, objName);

							let colorMap = new Texture();
							if (mat.map != null) {
								colorMap.CreateImageTexture(renderer.gl, mat.map.image);
							}
							else {
								colorMap.CreateConstantTexture(renderer.gl, mat.color.toArray());
							}

							let material, shadowMaterial;
							let Translation = [transform.modelTransX, transform.modelTransY, transform.modelTransZ];
							let Rotation = [transform.modelRotateX, transform.modelRotateY, transform.modelRotateZ];
							let Scale = [transform.modelScaleX, transform.modelScaleY, transform.modelScaleZ];

							for(let i = 0; i < renderer.lights.length; i++){
								let light = renderer.lights[i].entity;
								switch (objMaterial) {
									case 'PhongMaterial':
										material = buildPhongMaterial(colorMap, mat.specular.toArray(), light, Translation, Rotation, Scale, i, "./webgl/shaders/phong_vs.glsl", "./webgl/shaders/phong_fs.glsl");
										shadowMaterial = buildShadowMaterial(light, Translation, Rotation, Scale, i, "./webgl/shaders/shadow_vs.glsl", "./webgl/shaders/shadow_fs.glsl");
										break;
								}
							
								material.then((data) => {
									let meshRender = new MeshRender(renderer.gl, mesh, data);
									renderer.addMeshRender(meshRender);
								});
								shadowMaterial.then((data) => {
									let shadowMeshRender = new MeshRender(renderer.gl, mesh, data);
									renderer.addShadowMeshRender(shadowMeshRender);
								});
							}
						}
					});
				}, onProgress, onError);
		});
}
