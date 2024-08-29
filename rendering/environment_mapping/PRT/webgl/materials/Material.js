class Material {
    #flatten_uniforms;  // is Map.  [uniform_name, { type, value }]
    #flatten_attribs;   // is Array. Vertex attribute
    #vsSrc;
    #fsSrc;
    constructor(uniforms, attribs, vsSrc, fsSrc, frameBuffer) {
        this.uniforms = uniforms;
        this.attribs = attribs;
        this.#vsSrc = vsSrc;
        this.#fsSrc = fsSrc;
        
        this.#flatten_uniforms = ['uViewMatrix','uModelMatrix', 'uProjectionMatrix', 'uCameraPos', 'uLightPos'];
        for (let k in uniforms) {
            this.#flatten_uniforms.push(k);
        }
        this.#flatten_attribs = attribs;

        this.frameBuffer = frameBuffer;
    }

    setMeshAttribs(extraAttribs) {
    
        for (let i = 0; i < extraAttribs.length; i++) {
            this.#flatten_attribs.push(extraAttribs[i]);
        }
    }

    compile(gl) {
        return new Shader(gl, this.#vsSrc, this.#fsSrc,
            {
                uniforms: this.#flatten_uniforms,
                attribs: this.#flatten_attribs
            });
    }
}