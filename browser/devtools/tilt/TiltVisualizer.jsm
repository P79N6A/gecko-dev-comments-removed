








































"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

const ELEMENT_MIN_SIZE = 4;
const INVISIBLE_ELEMENTS = {
  "head": true,
  "base": true,
  "basefont": true,
  "isindex": true,
  "link": true,
  "meta": true,
  "option": true,
  "script": true,
  "style": true,
  "title": true
};

const STACK_THICKNESS = 15;
const WIREFRAME_COLOR = [0, 0, 0, 0.25];
const INITIAL_TRANSITION_DURATION = 100;
const INITIAL_Z_TRANSLATION = 400;

const MOUSE_CLICK_THRESHOLD = 10;
const ARCBALL_SENSITIVITY = 0.3;
const ARCBALL_ZOOM_STEP = 0.1;
const ARCBALL_ROTATION_STEP = 0.15;
const ARCBALL_TRANSLATION_STEP = 35;
const ARCBALL_SCROLL_MIN = -3000;
const ARCBALL_SCROLL_MAX = 500;

const TILT_CRAFTER = "resource:///modules/devtools/TiltWorkerCrafter.js";
const TILT_PICKER = "resource:///modules/devtools/TiltWorkerPicker.js";

Cu.import("resource:///modules/devtools/TiltGL.jsm");
Cu.import("resource:///modules/devtools/TiltMath.jsm");
Cu.import("resource:///modules/devtools/TiltUtils.jsm");
Cu.import("resource:///modules/devtools/TiltVisualizerStyle.jsm");

let EXPORTED_SYMBOLS = ["TiltVisualizer"];













function TiltVisualizer(aProperties)
{
  
  aProperties = aProperties || {};

  


  this.canvas = TiltUtils.DOM.initCanvas(aProperties.parentNode, {
    focusable: true,
    append: true
  });

  


  this.presenter = new TiltVisualizer.Presenter(this.canvas,
    aProperties.contentWindow,
    aProperties.requestAnimationFrame,
    aProperties.inspectorUI,
    aProperties.onError || null,
    aProperties.onLoad || null);

  


  this.controller = new TiltVisualizer.Controller(this.canvas, this.presenter);
}

TiltVisualizer.prototype = {

  




  isInitialized: function TV_isInitialized()
  {
    return this.presenter && this.presenter.isInitialized() &&
           this.controller && this.controller.isInitialized();
  },

  


  removeOverlay: function TV_removeOverlay()
  {
    if (this.canvas && this.canvas.parentNode) {
      this.canvas.parentNode.removeChild(this.canvas);
    }
  },

  


  cleanup: function TV_cleanup()
  {
    if (this.controller) {
      TiltUtils.destroyObject(this.controller);
    }
    if (this.presenter) {
      TiltUtils.destroyObject(this.presenter);
    }
    TiltUtils.destroyObject(this);
    TiltUtils.clearCache();
    TiltUtils.gc();
  }
};

















TiltVisualizer.Presenter = function TV_Presenter(
  aCanvas, aContentWindow, aRequestAnimationFrame, aInspectorUI,
  onError, onLoad)
{
  this.canvas = aCanvas;
  this.contentWindow = aContentWindow;
  this.inspectorUI = aInspectorUI;

  


  this.renderer = new TiltGL.Renderer(aCanvas, onError, onLoad);

  


  this.visualizationProgram = null;

  


  this.texture = null;
  this.meshStacks = null;
  this.meshWireframe = null;
  this.traverseData = null;

  


  this.highlight = {
    disabled: true,
    v0: vec3.create(),
    v1: vec3.create(),
    v2: vec3.create(),
    v3: vec3.create()
  };

  



  this.transforms = {
    offset: vec3.create(),      
    translation: vec3.create(), 
    rotation: quat4.create()    
  };

  


  this._initialSelection = false; 
  this._currentSelection = -1; 

  



  this.redraw = true;

  


  this.frames = 0;

  


  let setup = function TVP_setup()
  {
    let renderer = this.renderer;

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    this.visualizationProgram = new renderer.Program({
      vs: TiltVisualizer.MeshShader.vs,
      fs: TiltVisualizer.MeshShader.fs,
      attributes: ["vertexPosition", "vertexTexCoord", "vertexColor"],
      uniforms: ["mvMatrix", "projMatrix", "sampler"]
    });

    this.setupTexture();
    this.setupMeshData();
    this.setupEventListeners();
    this.canvas.focus();
  }.bind(this);

  


  let loop = function TVP_loop()
  {
    let renderer = this.renderer;

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    aRequestAnimationFrame(loop);

    
    if (this.redraw) {
      this.redraw = false;
      this.drawVisualization();
    }

    
    if ("function" === typeof this.ondraw) {
      this.ondraw();
    }
  }.bind(this);

  setup();
  loop();
};

TiltVisualizer.Presenter.prototype = {

  


  drawVisualization: function TVP_drawVisualization()
  {
    let renderer = this.renderer;
    let transforms = this.transforms;

    
    if (!this.meshStacks || !this.meshWireframe) {
      return;
    }

    
    renderer.clear();
    renderer.perspective();

    
    let f = this.frames / INITIAL_TRANSITION_DURATION;
    let w = renderer.width;
    let h = renderer.height;
    renderer.lerp(renderer.projMatrix, mat4.ortho(0, w, h, 0, -1, 1000), f, 8);

    
    renderer.translate(w * 0.5, h * 0.5, -INITIAL_Z_TRANSLATION);

    
    renderer.translate(transforms.translation[0], 0,
                       transforms.translation[2]);

    renderer.transform(quat4.toMat4(transforms.rotation));

    
    renderer.translate(transforms.offset[0],
                       transforms.offset[1] + transforms.translation[1], 0);

    
    renderer.strokeWeight(2);
    renderer.depthTest(true);
    this.drawMeshStacks();
    this.drawMeshWireframe();
    this.drawHighlight();

    
    if (this.frames < INITIAL_TRANSITION_DURATION) {
      this.redraw = true;
    }
    this.frames++;
  },

  


  drawMeshStacks: function TVP_drawMeshStacks()
  {
    let renderer = this.renderer;
    let mesh = this.meshStacks;

    let visualizationProgram = this.visualizationProgram;
    let texture = this.texture;
    let mvMatrix = renderer.mvMatrix;
    let projMatrix = renderer.projMatrix;

    
    visualizationProgram.use();

    
    visualizationProgram.bindVertexBuffer("vertexPosition", mesh.vertices);
    visualizationProgram.bindVertexBuffer("vertexTexCoord", mesh.texCoord);
    visualizationProgram.bindVertexBuffer("vertexColor", mesh.color);

    visualizationProgram.bindUniformMatrix("mvMatrix", mvMatrix);
    visualizationProgram.bindUniformMatrix("projMatrix", projMatrix);
    visualizationProgram.bindTexture("sampler", texture);

    
    renderer.drawIndexedVertices(renderer.context.TRIANGLES, mesh.indices);

    
    mesh.mvMatrix = mat4.create(mvMatrix);
    mesh.projMatrix = mat4.create(projMatrix);
  },

  


  drawMeshWireframe: function TVP_drawMeshWireframe()
  {
    let renderer = this.renderer;
    let mesh = this.meshWireframe;

    
    renderer.useColorShader(mesh.vertices, WIREFRAME_COLOR);

    
    renderer.drawIndexedVertices(renderer.context.LINES, mesh.indices);
  },

  


  drawHighlight: function TVP_drawHighlight()
  {
    
    if (!this.highlight.disabled) {

      
      let renderer = this.renderer;
      let highlight = this.highlight;

      renderer.depthTest(false);
      renderer.fill(highlight.fill, 0.5);
      renderer.stroke(highlight.stroke);
      renderer.strokeWeight(highlight.strokeWeight);
      renderer.quad(highlight.v0, highlight.v1, highlight.v2, highlight.v3);
    }
  },

  


  setupTexture: function TVP_setupTexture()
  {
    let renderer = this.renderer;

    
    TiltUtils.destroyObject(this.texture);

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    this.maxTextureSize =
      renderer.context.getParameter(renderer.context.MAX_TEXTURE_SIZE);

    
    
    
    
    this.texture = new renderer.Texture({
      source: TiltGL.TextureUtils.createContentImage(this.contentWindow,
                                                     this.maxTextureSize),
      format: "RGB"
    });
  },

  






  setupMesh: function TVP_setupMesh(aData)
  {
    let renderer = this.renderer;

    
    TiltUtils.destroyObject(this.meshStacks);
    TiltUtils.destroyObject(this.meshWireframe);

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    
    this.meshStacks = {
      vertices: new renderer.VertexBuffer(aData.vertices, 3),
      texCoord: new renderer.VertexBuffer(aData.texCoord, 2),
      color: new renderer.VertexBuffer(aData.color, 3),
      indices: new renderer.IndexBuffer(aData.stacksIndices)
    };

    
    
    this.meshWireframe = {
      vertices: this.meshStacks.vertices,
      indices: new renderer.IndexBuffer(aData.wireframeIndices)
    };

    
    if (!this._initialSelection) {
      this._initialSelection = true;
      this.highlightNode(this.inspectorUI.selection);
    }

    let width = renderer.width;
    let height = renderer.height;

    
    this.transforms.offset[0] = -Math.min(aData.meshWidth, width) * 0.5;
    this.transforms.offset[1] = -Math.min(aData.meshHeight, height) * 0.5;

    
    this.canvas.style.background = TiltVisualizerStyle.canvas.background;

    this.drawVisualization();
    this.redraw = true;
  },

  


  setupMeshData: function TVP_setupMeshData()
  {
    let renderer = this.renderer;

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    this.traverseData = TiltUtils.DOM.traverse(this.contentWindow, {
      invisibleElements: INVISIBLE_ELEMENTS,
      minSize: ELEMENT_MIN_SIZE,
      maxX: this.texture.width,
      maxY: this.texture.height
    });

    let worker = new ChromeWorker(TILT_CRAFTER);

    worker.addEventListener("message", function TVP_onMessage(event) {
      this.setupMesh(event.data);
    }.bind(this), false);

    
    
    worker.postMessage({
      thickness: STACK_THICKNESS,
      style: TiltVisualizerStyle.nodes,
      texWidth: this.texture.width,
      texHeight: this.texture.height,
      nodesInfo: this.traverseData.info
    });
  },

  


  setupEventListeners: function TVP_setupEventListeners()
  {
    
    TiltUtils.bindObjectFunc(this, "^on");

    this.contentWindow.addEventListener("resize", this.onResize, false);
  },

  


  onResize: function TVP_onResize(e)
  {
    let width = e.target.innerWidth;
    let height = e.target.innerHeight;

    
    this.renderer.width = width;
    this.renderer.height = height;

    this.redraw = true;
  },

  





  highlightNode: function TVP_highlightNode(aNode)
  {
    if (!aNode) {
      return;
    }

    this.highlightNodeFor(this.traverseData.nodes.indexOf(aNode));
  },

  








  highlightNodeAt: function TVP_highlightNodeAt(x, y)
  {
    
    this.pickNode(x, y, {

      


      onfail: function TVP_onHighlightFail()
      {
        this.highlightNodeFor(-1);
      }.bind(this),

      





      onpick: function TVP_onHighlightPick(aIntersection)
      {
        this.highlightNodeFor(aIntersection.index);
      }.bind(this)
    });
  },

  






  highlightNodeFor: function TVP_highlightNodeFor(aNodeIndex)
  {
    this.redraw = true;

    
    if (this._currentSelection === aNodeIndex) {
      return;
    }
    
    if (aNodeIndex < 0) {
      this._currentSelection = -1;
      this.highlight.disabled = true;
      return;
    }

    let highlight = this.highlight;
    let info = this.traverseData.info[aNodeIndex];
    let node = this.traverseData.nodes[aNodeIndex];
    let style = TiltVisualizerStyle.nodes;

    highlight.disabled = false;
    highlight.fill = style[info.name] || style.highlight.defaultFill;
    highlight.stroke = style.highlight.defaultStroke;
    highlight.strokeWeight = style.highlight.defaultStrokeWeight;

    let x = info.coord.left;
    let y = info.coord.top;
    let w = info.coord.width;
    let h = info.coord.height;
    let z = info.depth;

    vec3.set([x,     y,     z * STACK_THICKNESS], highlight.v0);
    vec3.set([x + w, y,     z * STACK_THICKNESS], highlight.v1);
    vec3.set([x + w, y + h, z * STACK_THICKNESS], highlight.v2);
    vec3.set([x,     y + h, z * STACK_THICKNESS], highlight.v3);

    this._currentSelection = aNodeIndex;
    this.inspectorUI.inspectNode(node);
  },

  












  pickNode: function TVP_pickNode(x, y, aProperties)
  {
    
    aProperties = aProperties || {};

    
    if (!this.meshStacks || !this.meshWireframe) {
      return;
    }

    let worker = new ChromeWorker(TILT_PICKER);

    worker.addEventListener("message", function TVP_onMessage(event) {
      if (event.data) {
        if ("function" === typeof aProperties.onpick) {
          aProperties.onpick(event.data);
        }
      } else {
        if ("function" === typeof aProperties.onfail) {
          aProperties.onfail();
        }
      }
    }, false);

    let width = this.renderer.width;
    let height = this.renderer.height;
    let mesh = this.meshStacks;

    
    
    
    worker.postMessage({
      thickness: STACK_THICKNESS,
      vertices: mesh.vertices.components,

      
      ray: vec3.createRay([x, y, 0], [x, y, 1], [0, 0, width, height],
        mesh.mvMatrix,
        mesh.projMatrix)
    });
  },

  





  setTranslation: function TVP_setTranslation(aTranslation)
  {
    let x = aTranslation[0];
    let y = aTranslation[1];
    let z = aTranslation[2];
    let transforms = this.transforms;

    
    if (transforms.translation[0] !== x ||
        transforms.translation[1] !== y ||
        transforms.translation[2] !== z) {

      vec3.set(aTranslation, transforms.translation);
      this.redraw = true;
    }
  },

  





  setRotation: function TVP_setRotation(aQuaternion)
  {
    let x = aQuaternion[0];
    let y = aQuaternion[1];
    let z = aQuaternion[2];
    let w = aQuaternion[3];
    let transforms = this.transforms;

    
    if (transforms.rotation[0] !== x ||
        transforms.rotation[1] !== y ||
        transforms.rotation[2] !== z ||
        transforms.rotation[3] !== w) {

      quat4.set(aQuaternion, transforms.rotation);
      this.redraw = true;
    }
  },

  




  isInitialized: function TVP_isInitialized()
  {
    return this.renderer && this.renderer.context;
  },

  


  finalize: function TVP_finalize()
  {
    TiltUtils.destroyObject(this.visualizationProgram);
    TiltUtils.destroyObject(this.texture);

    if (this.meshStacks) {
      TiltUtils.destroyObject(this.meshStacks.vertices);
      TiltUtils.destroyObject(this.meshStacks.texCoord);
      TiltUtils.destroyObject(this.meshStacks.color);
      TiltUtils.destroyObject(this.meshStacks.indices);
    }

    if (this.meshWireframe) {
      TiltUtils.destroyObject(this.meshWireframe.indices);
    }

    TiltUtils.destroyObject(this.highlight);
    TiltUtils.destroyObject(this.transforms);
    TiltUtils.destroyObject(this.renderer);

    this.contentWindow.removeEventListener("resize", this.onResize, false);
  }
};









TiltVisualizer.Controller = function TV_Controller(aCanvas, aPresenter)
{
  this.canvas = aCanvas;
  this.presenter = aPresenter;

  


  this.left = aPresenter.contentWindow.pageXOffset || 0;
  this.top = aPresenter.contentWindow.pageYOffset || 0;
  this.width = aCanvas.width;
  this.height = aCanvas.height;

  


  this.arcball = new TiltVisualizer.Arcball(this.width, this.height, 0,
    [this.width + this.left < aPresenter.maxTextureSize ? -this.left : 0,
     this.height + this.top < aPresenter.maxTextureSize ? -this.top : 0]);

  


  this.coordinates = null;

  
  TiltUtils.bindObjectFunc(this, "update");
  TiltUtils.bindObjectFunc(this, "^on");

  
  aCanvas.addEventListener("mousedown", this.onMouseDown, false);
  aCanvas.addEventListener("mouseup", this.onMouseUp, false);
  aCanvas.addEventListener("click", this.onMouseClick, false);
  aCanvas.addEventListener("mousemove", this.onMouseMove, false);
  aCanvas.addEventListener("mouseover", this.onMouseOver, false);
  aCanvas.addEventListener("mouseout", this.onMouseOut, false);
  aCanvas.addEventListener("MozMousePixelScroll", this.onMozScroll, false);
  aCanvas.addEventListener("keydown", this.onKeyDown, false);
  aCanvas.addEventListener("keyup", this.onKeyUp, false);

  
  aPresenter.contentWindow.addEventListener("resize", this.onResize, false);

  
  aPresenter.ondraw = this.update;
};

TiltVisualizer.Controller.prototype = {

  


  update: function TVC_update()
  {
    this.coordinates = this.arcball.update();

    this.presenter.setRotation(this.coordinates.rotation);
    this.presenter.setTranslation(this.coordinates.translation);
  },

  


  onMouseDown: function TVC_onMouseDown(e)
  {
    e.target.focus();
    e.preventDefault();
    e.stopPropagation();

    
    this._downX = e.clientX - e.target.offsetLeft;
    this._downY = e.clientY - e.target.offsetTop;

    this.arcball.mouseDown(this._downX, this._downY, e.which);
  },

  


  onMouseUp: function TVC_onMouseUp(e)
  {
    e.preventDefault();
    e.stopPropagation();

    
    let button = e.which;
    let upX = e.clientX - e.target.offsetLeft;
    let upY = e.clientY - e.target.offsetTop;

    this.arcball.mouseUp(upX, upY, button);
  },

  


  onMouseClick: function TVC_onMouseClick(e)
  {
    e.preventDefault();
    e.stopPropagation();

    
    let button = e.which;
    let clickX = e.clientX - e.target.offsetLeft;
    let clickY = e.clientY - e.target.offsetTop;

    
    
    if (Math.abs(this._downX - clickX) < MOUSE_CLICK_THRESHOLD &&
        Math.abs(this._downY - clickY) < MOUSE_CLICK_THRESHOLD) {

      this.presenter.highlightNodeAt(clickX, clickY);
    }
  },

  


  onMouseMove: function TVC_onMouseMove(e)
  {
    e.preventDefault();
    e.stopPropagation();

    
    let moveX = e.clientX - e.target.offsetLeft;
    let moveY = e.clientY - e.target.offsetTop;

    this.arcball.mouseMove(moveX, moveY);
  },

  


  onMouseOver: function TVC_onMouseOver(e)
  {
    e.preventDefault();
    e.stopPropagation();

    this.arcball.mouseOver();
  },

  


  onMouseOut: function TVC_onMouseOut(e)
  {
    e.preventDefault();
    e.stopPropagation();

    this.arcball.mouseOut();
  },

  


  onMozScroll: function TVC_onMozScroll(e)
  {
    e.preventDefault();
    e.stopPropagation();

    this.arcball.mouseScroll(e.detail);
  },

  


  onKeyDown: function TVC_onKeyDown(e)
  {
    let code = e.keyCode || e.which;

    if (code >= e.DOM_VK_LEFT && code <= e.DOM_VK_DOWN) {
      e.preventDefault();
      e.stopPropagation();
    }
    this.arcball.keyDown(code);
  },

  


  onKeyUp: function TVC_onKeyUp(e)
  {
    let code = e.keyCode || e.which;

    if (code >= e.DOM_VK_LEFT && code <= e.DOM_VK_DOWN) {
      e.preventDefault();
      e.stopPropagation();
    }
    this.arcball.keyUp(code);
  },

  


  onResize: function TVC_onResize(e)
  {
    let width = e.target.innerWidth;
    let height = e.target.innerHeight;

    this.arcball.resize(width, height);
  },

  




  isInitialized: function TVC_isInitialized()
  {
    return this.arcball ? true : false;
  },

  


  finalize: function TVC_finalize()
  {
    let canvas = this.canvas;
    let presenter = this.presenter;

    TiltUtils.destroyObject(this.arcball);
    TiltUtils.destroyObject(this.coordinates);

    canvas.removeEventListener("mousedown", this.onMouseDown, false);
    canvas.removeEventListener("mouseup", this.onMouseUp, false);
    canvas.removeEventListener("click", this.onMouseClick, false);
    canvas.removeEventListener("mousemove", this.onMouseMove, false);
    canvas.removeEventListener("mouseover", this.onMouseOver, false);
    canvas.removeEventListener("mouseout", this.onMouseOut, false);
    canvas.removeEventListener("MozMousePixelScroll", this.onMozScroll, false);
    canvas.removeEventListener("keydown", this.onKeyDown, false);
    canvas.removeEventListener("keyup", this.onKeyUp, false);
    presenter.contentWindow.removeEventListener("resize", this.onResize,false);
    presenter.ondraw = null;
  }
};

















TiltVisualizer.Arcball = function TV_Arcball(
  aWidth, aHeight, aRadius, aInitialTrans, aInitialRot)
{
  


  this._mousePress = vec3.create();
  this._mouseRelease = vec3.create();
  this._mouseMove = vec3.create();
  this._mouseLerp = vec3.create();

  


  this._mouseButton = -1;
  this._scrollValue = 0;

  


  this._keyCode = {};

  




  this._startVec = vec3.create();
  this._endVec = vec3.create();
  this._pVec = vec3.create();

  


  this._lastRot = quat4.create();
  this._deltaRot = quat4.create();
  this._currentRot = quat4.create(aInitialRot);

  


  this._lastTrans = vec3.create();
  this._deltaTrans = vec3.create();
  this._currentTrans = vec3.create(aInitialTrans);

  


  this._addKeyRot = vec3.create();
  this._addKeyTrans = vec3.create();
  this._deltaKeyRot = quat4.create();
  this._deltaKeyTrans = vec3.create();

  
  this.resize(aWidth, aHeight, aRadius);
};

TiltVisualizer.Arcball.prototype = {

  






  update: function TVA_update()
  {
    let mousePress = this._mousePress;
    let mouseRelease = this._mouseRelease;
    let mouseMove = this._mouseMove;
    let mouseLerp = this._mouseLerp;
    let mouseButton = this._mouseButton;

    
    mouseLerp[0] += (mouseMove[0] - mouseLerp[0]) * ARCBALL_SENSITIVITY;
    mouseLerp[1] += (mouseMove[1] - mouseLerp[1]) * ARCBALL_SENSITIVITY;

    
    let x = mouseLerp[0];
    let y = mouseLerp[1];

    
    
    
    if (mouseButton === 3 || x === mouseRelease[0] && y === mouseRelease[1]) {
      this._rotating = false;
    }

    let startVec = this._startVec;
    let endVec = this._endVec;
    let pVec = this._pVec;

    let lastRot = this._lastRot;
    let deltaRot = this._deltaRot;
    let currentRot = this._currentRot;

    
    if (mouseButton === 1 || this._rotating) {
      
      
      this._rotating = true;

      
      this.pointToSphere(x, y, this.width, this.height, this.radius, endVec);

      
      vec3.cross(startVec, endVec, pVec);

      
      if (vec3.length(pVec) > 0) {
        deltaRot[0] = pVec[0];
        deltaRot[1] = pVec[1];
        deltaRot[2] = pVec[2];

        
        
        deltaRot[3] = -vec3.dot(startVec, endVec);
      } else {
        
        deltaRot[0] = 0;
        deltaRot[1] = 0;
        deltaRot[2] = 0;
        deltaRot[3] = 1;
      }

      
      quat4.multiply(lastRot, deltaRot, currentRot);
    } else {
      
      quat4.set(currentRot, lastRot);
    }

    let lastTrans = this._lastTrans;
    let deltaTrans = this._deltaTrans;
    let currentTrans = this._currentTrans;

    
    if (mouseButton === 3) {
      
      
      deltaTrans[0] = mouseMove[0] - mousePress[0];
      deltaTrans[1] = mouseMove[1] - mousePress[1];

      currentTrans[0] = lastTrans[0] + deltaTrans[0];
      currentTrans[1] = lastTrans[1] + deltaTrans[1];
    } else {
      
      lastTrans[0] = currentTrans[0];
      lastTrans[1] = currentTrans[1];
    }

    let scrollValue = this._scrollValue;
    let keyCode = this._keyCode;

    
    deltaTrans[2] = (scrollValue - currentTrans[2]) * ARCBALL_ZOOM_STEP;
    currentTrans[2] += deltaTrans[2];

    let addKeyRot = this._addKeyRot;
    let addKeyTrans = this._addKeyTrans;
    let deltaKeyRot = this._deltaKeyRot;
    let deltaKeyTrans = this._deltaKeyTrans;

    
    if (keyCode[Ci.nsIDOMKeyEvent.DOM_VK_A]) {
      addKeyRot[0] -= ARCBALL_SENSITIVITY * ARCBALL_ROTATION_STEP;
    }
    if (keyCode[Ci.nsIDOMKeyEvent.DOM_VK_D]) {
      addKeyRot[0] += ARCBALL_SENSITIVITY * ARCBALL_ROTATION_STEP;
    }
    if (keyCode[Ci.nsIDOMKeyEvent.DOM_VK_W]) {
      addKeyRot[1] += ARCBALL_SENSITIVITY * ARCBALL_ROTATION_STEP;
    }
    if (keyCode[Ci.nsIDOMKeyEvent.DOM_VK_S]) {
      addKeyRot[1] -= ARCBALL_SENSITIVITY * ARCBALL_ROTATION_STEP;
    }
    if (keyCode[Ci.nsIDOMKeyEvent.DOM_VK_LEFT]) {
      addKeyTrans[0] -= ARCBALL_SENSITIVITY * ARCBALL_TRANSLATION_STEP;
    }
    if (keyCode[Ci.nsIDOMKeyEvent.DOM_VK_RIGHT]) {
      addKeyTrans[0] += ARCBALL_SENSITIVITY * ARCBALL_TRANSLATION_STEP;
    }
    if (keyCode[Ci.nsIDOMKeyEvent.DOM_VK_UP]) {
      addKeyTrans[1] -= ARCBALL_SENSITIVITY * ARCBALL_TRANSLATION_STEP;
    }
    if (keyCode[Ci.nsIDOMKeyEvent.DOM_VK_DOWN]) {
      addKeyTrans[1] += ARCBALL_SENSITIVITY * ARCBALL_TRANSLATION_STEP;
    }

    
    deltaKeyRot[0] +=
      (addKeyRot[0] - deltaKeyRot[0]) * ARCBALL_SENSITIVITY;
    deltaKeyRot[1] +=
      (addKeyRot[1] - deltaKeyRot[1]) * ARCBALL_SENSITIVITY;

    deltaKeyTrans[0] +=
      (addKeyTrans[0] - deltaKeyTrans[0]) * ARCBALL_SENSITIVITY;
    deltaKeyTrans[1] +=
      (addKeyTrans[1] - deltaKeyTrans[1]) * ARCBALL_SENSITIVITY;

    
    quat4.fromEuler(deltaKeyRot[0], deltaKeyRot[1], 0, deltaRot);

    
    deltaTrans[0] = deltaKeyTrans[0];
    deltaTrans[1] = deltaKeyTrans[1];
    deltaTrans[2] = 0;

    
    return {
      rotation: quat4.multiply(deltaRot, currentRot),
      translation: vec3.add(deltaTrans, currentTrans)
    };
  },

  










  mouseDown: function TVA_mouseDown(x, y, aButton)
  {
    
    this._mousePress[0] = x;
    this._mousePress[1] = y;
    this._mouseButton = aButton;
    this._save();

    
    this.pointToSphere(
      x, y, this.width, this.height, this.radius, this._startVec);

    quat4.set(this._currentRot, this._lastRot);
  },

  








  mouseUp: function TVA_mouseUp(x, y)
  {
    
    this._mouseRelease[0] = x;
    this._mouseRelease[1] = y;
    this._mouseButton = -1;
  },

  








  mouseMove: function TVA_mouseMove(x, y)
  {
    
    
    if (this._mouseButton !== -1) {
      this._mouseMove[0] = x;
      this._mouseMove[1] = y;
    }
  },

  



  mouseOver: function TVA_mouseOver()
  {
    
    this._mouseButton = -1;
  },

  



  mouseOut: function TVA_mouseOut()
  {
    
    this._mouseButton = -1;
  },

  






  mouseScroll: function TVA_mouseScroll(aScroll)
  {
    this._scrollValue = TiltMath.clamp(this._scrollValue - aScroll,
      ARCBALL_SCROLL_MIN, ARCBALL_SCROLL_MAX);
  },

  






  keyDown: function TVA_keyDown(aCode)
  {
    this._keyCode[aCode] = true;
  },

  






  keyUp: function TVA_keyUp(aCode)
  {
    this._keyCode[aCode] = false;
  },

  















  pointToSphere: function TVA_pointToSphere(
    x, y, aWidth, aHeight, aRadius, aSphereVec)
  {
    
    x = (x - aWidth * 0.5) / aRadius;
    y = (y - aHeight * 0.5) / aRadius;

    
    let normal = 0;
    let sqlength = x * x + y * y;

    
    if (sqlength > 1) {
      
      normal = 1 / Math.sqrt(sqlength);

      
      aSphereVec[0] = x * normal;
      aSphereVec[1] = y * normal;
      aSphereVec[2] = 0;
    } else {
      
      aSphereVec[0] = x;
      aSphereVec[1] = y;
      aSphereVec[2] = Math.sqrt(1 - sqlength);
    }
  },

  










  resize: function TVA_resize(newWidth, newHeight, newRadius)
  {
    if (!newWidth || !newHeight) {
      return;
    }

    
    this.width = newWidth;
    this.height = newHeight;
    this.radius = newRadius ? newRadius : Math.min(newWidth, newHeight);
    this._save();
  },

  


  _save: function TVA__save()
  {
    let x = this._mousePress[0];
    let y = this._mousePress[1];

    this._mouseMove[0] = x;
    this._mouseMove[1] = y;
    this._mouseRelease[0] = x;
    this._mouseRelease[1] = y;
    this._mouseLerp[0] = x;
    this._mouseLerp[1] = y;
  }
};




TiltVisualizer.Prefs = {

  


  get enabled()
  {
    return this._enabled;
  },

  set enabled(value)
  {
    TiltUtils.Preferences.set("enabled", "boolean", value);
    this._enabled = value;
  },

  


  get forceEnabled()
  {
    return this._forceEnabled;
  },

  set forceEnabled(value)
  {
    TiltUtils.Preferences.set("force-enabled", "boolean", value);
    this._forceEnabled = value;
  },

  


  load: function TVC_load()
  {
    let prefs = TiltUtils.Preferences;

    TiltVisualizer.Prefs._enabled = prefs.get("enabled", "boolean");
    TiltVisualizer.Prefs._forceEnabled = prefs.get("force-enabled", "boolean");
  }
};











TiltVisualizer.MeshShader = {

  


  vs: [
    "attribute vec3 vertexPosition;",
    "attribute vec2 vertexTexCoord;",
    "attribute vec3 vertexColor;",

    "uniform mat4 mvMatrix;",
    "uniform mat4 projMatrix;",

    "varying vec2 texCoord;",
    "varying vec3 color;",

    "void main() {",
    "  gl_Position = projMatrix * mvMatrix * vec4(vertexPosition, 1.0);",
    "  texCoord = vertexTexCoord;",
    "  color = vertexColor;",
    "}"
  ].join("\n"),

  


  fs: [
    "#ifdef GL_ES",
    "precision lowp float;",
    "#endif",

    "uniform sampler2D sampler;",

    "varying vec2 texCoord;",
    "varying vec3 color;",

    "void main() {",
    "  if (texCoord.x < 0.0) {",
    "    gl_FragColor = vec4(color, 1.0);",
    "  } else {",
    "    gl_FragColor = vec4(texture2D(sampler, texCoord).rgb, 1.0);",
    "  }",
    "}"
  ].join("\n")
};
