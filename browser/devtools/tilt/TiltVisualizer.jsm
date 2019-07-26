




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






const MAX_GROUP_NODES = Math.pow(2, Uint16Array.BYTES_PER_ELEMENT * 8) / 12 - 1;

const STACK_THICKNESS = 15;
const WIREFRAME_COLOR = [0, 0, 0, 0.25];
const INTRO_TRANSITION_DURATION = 1000;
const OUTRO_TRANSITION_DURATION = 800;
const INITIAL_Z_TRANSLATION = 400;
const MOVE_INTO_VIEW_ACCURACY = 50;

const MOUSE_CLICK_THRESHOLD = 10;
const MOUSE_INTRO_DELAY = 200;
const ARCBALL_SENSITIVITY = 0.5;
const ARCBALL_ROTATION_STEP = 0.15;
const ARCBALL_TRANSLATION_STEP = 35;
const ARCBALL_ZOOM_STEP = 0.1;
const ARCBALL_ZOOM_MIN = -3000;
const ARCBALL_ZOOM_MAX = 500;
const ARCBALL_RESET_SPHERICAL_FACTOR = 0.1;
const ARCBALL_RESET_LINEAR_FACTOR = 0.01;

const TILT_CRAFTER = "resource:///modules/devtools/TiltWorkerCrafter.js";
const TILT_PICKER = "resource:///modules/devtools/TiltWorkerPicker.js";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource:///modules/devtools/Target.jsm");
Cu.import("resource:///modules/devtools/TiltGL.jsm");
Cu.import("resource:///modules/devtools/TiltMath.jsm");
Cu.import("resource:///modules/devtools/TiltUtils.jsm");
Cu.import("resource:///modules/devtools/TiltVisualizerStyle.jsm");

this.EXPORTED_SYMBOLS = ["TiltVisualizer"];













this.TiltVisualizer = function TiltVisualizer(aProperties)
{
  
  aProperties = aProperties || {};

  


  this.chromeWindow = aProperties.chromeWindow;

  


  this.canvas = TiltUtils.DOM.initCanvas(aProperties.parentNode, {
    focusable: true,
    append: true
  });

  


  this.presenter = new TiltVisualizer.Presenter(this.canvas,
    aProperties.chromeWindow,
    aProperties.contentWindow,
    aProperties.notifications,
    aProperties.onError || null,
    aProperties.onLoad || null);

  this.bindToInspector(aProperties.tab);

  


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
    this.unbindInspector();

    if (this.controller) {
      TiltUtils.destroyObject(this.controller);
    }
    if (this.presenter) {
      TiltUtils.destroyObject(this.presenter);
    }

    let chromeWindow = this.chromeWindow;

    TiltUtils.destroyObject(this);
    TiltUtils.clearCache();
    TiltUtils.gc(chromeWindow);
  },

  


  bindToInspector: function TV_bindToInspector(aTab)
  {
    this._browserTab = aTab;

    this.onNewNodeFromInspector = this.onNewNodeFromInspector.bind(this);
    this.onNewNodeFromTilt = this.onNewNodeFromTilt.bind(this);
    this.onInspectorReady = this.onInspectorReady.bind(this);
    this.onToolboxDestroyed = this.onToolboxDestroyed.bind(this);

    gDevTools.on("inspector-ready", this.onInspectorReady);
    gDevTools.on("toolbox-destroyed", this.onToolboxDestroyed);

    Services.obs.addObserver(this.onNewNodeFromTilt,
                             this.presenter.NOTIFICATIONS.HIGHLIGHTING,
                             false);
    Services.obs.addObserver(this.onNewNodeFromTilt,
                             this.presenter.NOTIFICATIONS.UNHIGHLIGHTING,
                             false);

    let target = TargetFactory.forTab(aTab);
    let toolbox = gDevTools.getToolbox(target);
    if (toolbox) {
      let panel = toolbox.getPanel("inspector");
      if (panel) {
        this.inspector = panel;
        this.inspector.selection.on("new-node", this.onNewNodeFromInspector);
        this.onNewNodeFromInspector();
      }
    }
  },

  


  unbindInspector: function TV_unbindInspector()
  {
    this._browserTab = null;

    if (this.inspector) {
      if (this.inspector.selection) {
        this.inspector.selection.off("new-node", this.onNewNodeFromInspector);
      }
      this.inspector = null;
    }

    gDevTools.off("inspector-ready", this.onInspectorReady);
    gDevTools.off("toolbox-destroyed", this.onToolboxDestroyed);

    Services.obs.removeObserver(this.onNewNodeFromTilt,
                                this.presenter.NOTIFICATIONS.HIGHLIGHTING);
    Services.obs.removeObserver(this.onNewNodeFromTilt,
                                this.presenter.NOTIFICATIONS.UNHIGHLIGHTING);
  },

  


  onInspectorReady: function TV_onInspectorReady(event, toolbox, panel)
  {
    if (toolbox.target.tab === this._browserTab) {
      this.inspector = panel;
      this.inspector.selection.on("new-node", this.onNewNodeFromInspector);
      this.onNewNodeFromTilt();
    }
  },

  


  onToolboxDestroyed: function TV_onToolboxDestroyed(event, tab)
  {
    if (tab === this._browserTab &&
        this.inspector) {
      if (this.inspector.selection) {
        this.inspector.selection.off("new-node", this.onNewNodeFromInspector);
      }
      this.inspector = null;
    }
  },

  


  onNewNodeFromInspector: function TV_onNewNodeFromInspector()
  {
    if (this.inspector &&
        this.inspector.selection.reason != "tilt") {
      let selection = this.inspector.selection;
      let canHighlightNode = selection.isNode() &&
                              selection.isConnected() &&
                              selection.isElementNode();
      if (canHighlightNode) {
        this.presenter.highlightNode(selection.node);
      } else {
        this.presenter.highlightNodeFor(-1);
      }
    }
  },

  


  onNewNodeFromTilt: function TV_onNewNodeFromTilt()
  {
    if (!this.inspector) {
      return;
    }
    let nodeIndex = this.presenter._currentSelection;
    if (nodeIndex < 0) {
      this.inspector.selection.setNode(null, "tilt");
    }
    let node = this.presenter._traverseData.nodes[nodeIndex];
    this.inspector.selection.setNode(node, "tilt");
  },
};

















TiltVisualizer.Presenter = function TV_Presenter(
  aCanvas, aChromeWindow, aContentWindow, aNotifications, onError, onLoad)
{
  


  this.canvas = aCanvas;

  


  this.chromeWindow = aChromeWindow;

  


  this.contentWindow = aContentWindow;

  


  this.NOTIFICATIONS = aNotifications;

  


  this._renderer = new TiltGL.Renderer(aCanvas, onError, onLoad);

  


  this._visualizationProgram = null;

  


  this._texture = null;
  this._meshData = null;
  this._meshStacks = null;
  this._meshWireframe = null;
  this._traverseData = null;

  


  this._highlight = {
    disabled: true,
    v0: vec3.create(),
    v1: vec3.create(),
    v2: vec3.create(),
    v3: vec3.create()
  };

  



  this.transforms = {
    zoom: 1,
    offset: vec3.create(),      
    translation: vec3.create(), 
    rotation: quat4.create()    
  };

  


  this._currentSelection = -1; 
  this._initialMeshConfiguration = false; 

  



  this._redraw = true;

  



  this._time = 0;

  



  this._delta = 0;
  this._prevFrameTime = 0;
  this._currFrameTime = 0;


  this._setup();
  this._loop();
};

TiltVisualizer.Presenter.prototype = {

  


  _setup: function TVP__setup()
  {
    let renderer = this._renderer;

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    this._visualizationProgram = new renderer.Program({
      vs: TiltVisualizer.MeshShader.vs,
      fs: TiltVisualizer.MeshShader.fs,
      attributes: ["vertexPosition", "vertexTexCoord", "vertexColor"],
      uniforms: ["mvMatrix", "projMatrix", "sampler"]
    });

    
    this.transforms.zoom = this._getPageZoom();

    
    TiltUtils.bindObjectFunc(this, "^_on");
    TiltUtils.bindObjectFunc(this, "_loop");

    this._setupTexture();
    this._setupMeshData();
    this._setupEventListeners();
    this.canvas.focus();
  },

  



  _getPageZoom: function TVP__getPageZoom() {
    return this.contentWindow
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils)
      .fullZoom;
  },

  


  _loop: function TVP__loop()
  {
    let renderer = this._renderer;

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    this.chromeWindow.mozRequestAnimationFrame(this._loop);

    
    if (this._redraw) {
      this._redraw = false;
      this._drawVisualization();
    }

    
    if ("function" === typeof this._controllerUpdate) {
      this._controllerUpdate(this._time, this._delta);
    }

    this._handleFrameDelta();
    this._handleKeyframeNotifications();
  },

  


  _handleFrameDelta: function TVP__handleFrameDelta()
  {
    this._prevFrameTime = this._currFrameTime;
    this._currFrameTime = this.chromeWindow.mozAnimationStartTime;
    this._delta = this._currFrameTime - this._prevFrameTime;
  },

  


  _drawVisualization: function TVP__drawVisualization()
  {
    let renderer = this._renderer;
    let transforms = this.transforms;
    let w = renderer.width;
    let h = renderer.height;
    let ih = renderer.initialHeight;

    
    if (!this._meshStacks || !this._meshWireframe) {
      return;
    }

    
    renderer.clear();
    renderer.perspective();

    
    let ortho = mat4.ortho(0, w, h, 0, -1000, 1000);

    if (!this._isExecutingDestruction) {
      let f = this._time / INTRO_TRANSITION_DURATION;
      renderer.lerp(renderer.projMatrix, ortho, f, 8);
    } else {
      let f = this._time / OUTRO_TRANSITION_DURATION;
      renderer.lerp(renderer.projMatrix, ortho, 1 - f, 8);
    }

    
    renderer.translate(w * 0.5, ih * 0.5, -INITIAL_Z_TRANSLATION);

    
    renderer.translate(transforms.translation[0], 0,
                       transforms.translation[2]);

    renderer.transform(quat4.toMat4(transforms.rotation));

    
    renderer.translate(transforms.offset[0],
                       transforms.offset[1] + transforms.translation[1], 0);

    renderer.scale(transforms.zoom, transforms.zoom);

    
    renderer.strokeWeight(2);
    renderer.depthTest(true);
    this._drawMeshStacks();
    this._drawMeshWireframe();
    this._drawHighlight();

    
    if (this._time < INTRO_TRANSITION_DURATION ||
        this._time < OUTRO_TRANSITION_DURATION) {
      this._redraw = true;
    }
    this._time += this._delta;
  },

  


  _drawMeshStacks: function TVP__drawMeshStacks()
  {
    let renderer = this._renderer;
    let mesh = this._meshStacks;

    let visualizationProgram = this._visualizationProgram;
    let texture = this._texture;
    let mvMatrix = renderer.mvMatrix;
    let projMatrix = renderer.projMatrix;

    
    visualizationProgram.use();

    for (let i = 0, len = mesh.length; i < len; i++) {
      let group = mesh[i];

      
      visualizationProgram.bindVertexBuffer("vertexPosition", group.vertices);
      visualizationProgram.bindVertexBuffer("vertexTexCoord", group.texCoord);
      visualizationProgram.bindVertexBuffer("vertexColor", group.color);

      visualizationProgram.bindUniformMatrix("mvMatrix", mvMatrix);
      visualizationProgram.bindUniformMatrix("projMatrix", projMatrix);
      visualizationProgram.bindTexture("sampler", texture);

      
      renderer.drawIndexedVertices(renderer.context.TRIANGLES, group.indices);
    }

    
    mesh.mvMatrix = mat4.create(mvMatrix);
    mesh.projMatrix = mat4.create(projMatrix);
  },

  


  _drawMeshWireframe: function TVP__drawMeshWireframe()
  {
    let renderer = this._renderer;
    let mesh = this._meshWireframe;

    for (let i = 0, len = mesh.length; i < len; i++) {
      let group = mesh[i];

      
      renderer.useColorShader(group.vertices, WIREFRAME_COLOR);

      
      renderer.drawIndexedVertices(renderer.context.LINES, group.indices);
    }
  },

  


  _drawHighlight: function TVP__drawHighlight()
  {
    
    if (!this._highlight.disabled) {

      
      let renderer = this._renderer;
      let highlight = this._highlight;

      renderer.depthTest(false);
      renderer.fill(highlight.fill, 0.5);
      renderer.stroke(highlight.stroke);
      renderer.strokeWeight(highlight.strokeWeight);
      renderer.quad(highlight.v0, highlight.v1, highlight.v2, highlight.v3);
    }
  },

  


  _setupTexture: function TVP__setupTexture()
  {
    let renderer = this._renderer;

    
    TiltUtils.destroyObject(this._texture); this._texture = null;

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    this._maxTextureSize =
      renderer.context.getParameter(renderer.context.MAX_TEXTURE_SIZE);

    
    
    
    
    this._texture = new renderer.Texture({
      source: TiltGL.TextureUtils.createContentImage(this.contentWindow,
                                                     this._maxTextureSize),
      format: "RGB"
    });

    if ("function" === typeof this._onSetupTexture) {
      this._onSetupTexture();
      this._onSetupTexture = null;
    }
  },

  






  _setupMesh: function TVP__setupMesh(aMeshData)
  {
    let renderer = this._renderer;

    
    TiltUtils.destroyObject(this._meshStacks); this._meshStacks = [];
    TiltUtils.destroyObject(this._meshWireframe); this._meshWireframe = [];

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    this._meshData = aMeshData;

    
    for (let i = 0, len = aMeshData.groups.length; i < len; i++) {
      let group = aMeshData.groups[i];

      
      
      this._meshStacks.push({
        vertices: new renderer.VertexBuffer(group.vertices, 3),
        texCoord: new renderer.VertexBuffer(group.texCoord, 2),
        color: new renderer.VertexBuffer(group.color, 3),
        indices: new renderer.IndexBuffer(group.stacksIndices)
      });

      
      
      this._meshWireframe.push({
        vertices: this._meshStacks[i].vertices,
        indices: new renderer.IndexBuffer(group.wireframeIndices)
      });
    }

    
    if (!this._initialMeshConfiguration) {
      this._initialMeshConfiguration = true;

      
      this.transforms.offset[0] = -renderer.width * 0.5;
      this.transforms.offset[1] = -renderer.height * 0.5;

      
      this.canvas.style.background = TiltVisualizerStyle.canvas.background;

      this._drawVisualization();
      this._redraw = true;
    }

    if ("function" === typeof this._onSetupMesh) {
      this._onSetupMesh();
      this._onSetupMesh = null;
    }
  },

  


  _setupMeshData: function TVP__setupMeshData()
  {
    let renderer = this._renderer;

    
    if (!renderer || !renderer.context) {
      return;
    }

    
    this._traverseData = TiltUtils.DOM.traverse(this.contentWindow, {
      invisibleElements: INVISIBLE_ELEMENTS,
      minSize: ELEMENT_MIN_SIZE,
      maxX: this._texture.width,
      maxY: this._texture.height
    });

    let worker = new ChromeWorker(TILT_CRAFTER);

    worker.addEventListener("message", function TVP_onMessage(event) {
      this._setupMesh(event.data);
    }.bind(this), false);

    
    
    worker.postMessage({
      maxGroupNodes: MAX_GROUP_NODES,
      thickness: STACK_THICKNESS,
      style: TiltVisualizerStyle.nodes,
      texWidth: this._texture.width,
      texHeight: this._texture.height,
      nodesInfo: this._traverseData.info
    });
  },

  


  _setupEventListeners: function TVP__setupEventListeners()
  {
    this.contentWindow.addEventListener("resize", this._onResize, false);
  },

  


  _onResize: function TVP_onResize(e)
  {
    let zoom = this._getPageZoom();
    let width = e.target.innerWidth * zoom;
    let height = e.target.innerHeight * zoom;

    
    this._renderer.width = width;
    this._renderer.height = height;

    this._redraw = true;
  },

  







  highlightNode: function TVP_highlightNode(aNode, aFlags)
  {
    this.highlightNodeFor(this._traverseData.nodes.indexOf(aNode), aFlags);
  },

  












  highlightNodeAt: function TVP_highlightNodeAt(x, y, aProperties)
  {
    
    aProperties = aProperties || {};

    
    this.pickNode(x, y, {

      


      onfail: function TVP_onHighlightFail()
      {
        this.highlightNodeFor(-1);

        if ("function" === typeof aProperties.onfail) {
          aProperties.onfail();
        }
      }.bind(this),

      





      onpick: function TVP_onHighlightPick(aIntersection)
      {
        this.highlightNodeFor(aIntersection.index);

        if ("function" === typeof aProperties.onpick) {
          aProperties.onpick();
        }
      }.bind(this)
    });
  },

  








  highlightNodeFor: function TVP_highlightNodeFor(aNodeIndex, aFlags)
  {
    this._redraw = true;

    
    if (this._currentSelection === aNodeIndex) {
      return;
    }

    
    if (aNodeIndex < 0) {
      this._currentSelection = -1;
      this._highlight.disabled = true;

      Services.obs.notifyObservers(null, this.NOTIFICATIONS.UNHIGHLIGHTING, null);
      return;
    }

    let highlight = this._highlight;
    let info = this._traverseData.info[aNodeIndex];
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

    
    
    

    if (aFlags && aFlags.indexOf("moveIntoView") !== -1)
    {
      this.controller.arcball.moveIntoView(vec3.lerp(
        vec3.scale(this._highlight.v0, this.transforms.zoom, []),
        vec3.scale(this._highlight.v1, this.transforms.zoom, []), 0.5));
    }

    Services.obs.notifyObservers(null, this.NOTIFICATIONS.HIGHLIGHTING, null);
  },

  






  deleteNode: function TVP_deleteNode(aNodeIndex)
  {
    
    if ((aNodeIndex = aNodeIndex || this._currentSelection) < 1) {
      return;
    }

    let renderer = this._renderer;

    let groupIndex = parseInt(aNodeIndex / MAX_GROUP_NODES);
    let nodeIndex = parseInt((aNodeIndex + (groupIndex ? 1 : 0)) % MAX_GROUP_NODES);
    let group = this._meshStacks[groupIndex];
    let vertices = group.vertices.components;

    for (let i = 0, k = 36 * nodeIndex; i < 36; i++) {
      vertices[i + k] = 0;
    }

    group.vertices = new renderer.VertexBuffer(vertices, 3);
    this._highlight.disabled = true;
    this._redraw = true;

    Services.obs.notifyObservers(null, this.NOTIFICATIONS.NODE_REMOVED, null);
  },

  












  pickNode: function TVP_pickNode(x, y, aProperties)
  {
    
    aProperties = aProperties || {};

    
    if (!this._meshStacks || !this._meshWireframe) {
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

    let zoom = this._getPageZoom();
    let width = this._renderer.width * zoom;
    let height = this._renderer.height * zoom;
    x *= zoom;
    y *= zoom;

    
    
    
    worker.postMessage({
      thickness: STACK_THICKNESS,
      vertices: this._meshData.allVertices,

      
      ray: vec3.createRay([x, y, 0], [x, y, 1], [0, 0, width, height],
        this._meshStacks.mvMatrix,
        this._meshStacks.projMatrix)
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
      this._redraw = true;
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
      this._redraw = true;
    }
  },

  


  _handleKeyframeNotifications: function TV__handleKeyframeNotifications()
  {
    if (!TiltVisualizer.Prefs.introTransition && !this._isExecutingDestruction) {
      this._time = INTRO_TRANSITION_DURATION;
    }
    if (!TiltVisualizer.Prefs.outroTransition && this._isExecutingDestruction) {
      this._time = OUTRO_TRANSITION_DURATION;
    }

    if (this._time >= INTRO_TRANSITION_DURATION &&
       !this._isInitializationFinished &&
       !this._isExecutingDestruction) {

      this._isInitializationFinished = true;
      Services.obs.notifyObservers(null, this.NOTIFICATIONS.INITIALIZED, null);

      if ("function" === typeof this._onInitializationFinished) {
        this._onInitializationFinished();
      }
    }

    if (this._time >= OUTRO_TRANSITION_DURATION &&
       !this._isDestructionFinished &&
        this._isExecutingDestruction) {

      this._isDestructionFinished = true;
      Services.obs.notifyObservers(null, this.NOTIFICATIONS.BEFORE_DESTROYED, null);

      if ("function" === typeof this._onDestructionFinished) {
        this._onDestructionFinished();
      }
    }
  },

  






  executeDestruction: function TV_executeDestruction(aCallback)
  {
    if (!this._isExecutingDestruction) {
      this._isExecutingDestruction = true;
      this._onDestructionFinished = aCallback;

      
      
      

      if (this._time > OUTRO_TRANSITION_DURATION) {
        this._time = 0;
        this._redraw = true;
      } else {
        aCallback();
      }
    }
  },

  




  isInitialized: function TVP_isInitialized()
  {
    return this._renderer && this._renderer.context;
  },

  


  _finalize: function TVP__finalize()
  {
    TiltUtils.destroyObject(this._visualizationProgram);
    TiltUtils.destroyObject(this._texture);

    if (this._meshStacks) {
      this._meshStacks.forEach(function(group) {
        TiltUtils.destroyObject(group.vertices);
        TiltUtils.destroyObject(group.texCoord);
        TiltUtils.destroyObject(group.color);
        TiltUtils.destroyObject(group.indices);
      });
    }
    if (this._meshWireframe) {
      this._meshWireframe.forEach(function(group) {
        TiltUtils.destroyObject(group.indices);
      });
    }

    TiltUtils.destroyObject(this._renderer);

    
    
    if (this.contentWindow == this.chromeWindow.content) {
      this.contentWindow.removeEventListener("resize", this._onResize, false);
    }
  }
};









TiltVisualizer.Controller = function TV_Controller(aCanvas, aPresenter)
{
  


  this.canvas = aCanvas;

  


  this.presenter = aPresenter;
  this.presenter.controller = this;

  


  this._zoom = aPresenter.transforms.zoom;
  this._left = (aPresenter.contentWindow.pageXOffset || 0) * this._zoom;
  this._top = (aPresenter.contentWindow.pageYOffset || 0) * this._zoom;
  this._width = aCanvas.width;
  this._height = aCanvas.height;

  


  this.arcball = new TiltVisualizer.Arcball(
    this.presenter.chromeWindow, this._width, this._height, 0,
    [
      this._width + this._left < aPresenter._maxTextureSize ? -this._left : 0,
      this._height + this._top < aPresenter._maxTextureSize ? -this._top : 0
    ]);

  


  this._coordinates = null;

  
  TiltUtils.bindObjectFunc(this, "_update");
  TiltUtils.bindObjectFunc(this, "^_on");

  
  this.addEventListeners();

  
  this.presenter._controllerUpdate = this._update;
};

TiltVisualizer.Controller.prototype = {

  


  addEventListeners: function TVC_addEventListeners()
  {
    let canvas = this.canvas;
    let presenter = this.presenter;

    
    canvas.addEventListener("mousedown", this._onMouseDown, false);
    canvas.addEventListener("mouseup", this._onMouseUp, false);
    canvas.addEventListener("mousemove", this._onMouseMove, false);
    canvas.addEventListener("mouseover", this._onMouseOver, false);
    canvas.addEventListener("mouseout", this._onMouseOut, false);
    canvas.addEventListener("MozMousePixelScroll", this._onMozScroll, false);
    canvas.addEventListener("keydown", this._onKeyDown, false);
    canvas.addEventListener("keyup", this._onKeyUp, false);
    canvas.addEventListener("keypress", this._onKeyPress, true);
    canvas.addEventListener("blur", this._onBlur, false);

    
    presenter.contentWindow.addEventListener("resize", this._onResize, false);
  },

  


  removeEventListeners: function TVC_removeEventListeners()
  {
    let canvas = this.canvas;
    let presenter = this.presenter;

    canvas.removeEventListener("mousedown", this._onMouseDown, false);
    canvas.removeEventListener("mouseup", this._onMouseUp, false);
    canvas.removeEventListener("mousemove", this._onMouseMove, false);
    canvas.removeEventListener("mouseover", this._onMouseOver, false);
    canvas.removeEventListener("mouseout", this._onMouseOut, false);
    canvas.removeEventListener("MozMousePixelScroll", this._onMozScroll, false);
    canvas.removeEventListener("keydown", this._onKeyDown, false);
    canvas.removeEventListener("keyup", this._onKeyUp, false);
    canvas.removeEventListener("keypress", this._onKeyPress, true);
    canvas.removeEventListener("blur", this._onBlur, false);

    
    
    if (presenter.contentWindow == presenter.chromeWindow.content) {
      presenter.contentWindow.removeEventListener("resize", this._onResize, false);
    }
  },

  







  _update: function TVC__update(aTime, aDelta)
  {
    this._time = aTime;
    this._coordinates = this.arcball.update(aDelta);

    this.presenter.setRotation(this._coordinates.rotation);
    this.presenter.setTranslation(this._coordinates.translation);
  },

  


  _onMouseDown: function TVC__onMouseDown(e)
  {
    e.target.focus();
    e.preventDefault();
    e.stopPropagation();

    if (this._time < MOUSE_INTRO_DELAY) {
      return;
    }

    
    let button = e.which;
    this._downX = e.clientX - e.target.offsetLeft;
    this._downY = e.clientY - e.target.offsetTop;

    this.arcball.mouseDown(this._downX, this._downY, button);
  },

  


  _onMouseUp: function TVC__onMouseUp(e)
  {
    e.preventDefault();
    e.stopPropagation();

    if (this._time < MOUSE_INTRO_DELAY) {
      return;
    }

    
    let button = e.which;
    let upX = e.clientX - e.target.offsetLeft;
    let upY = e.clientY - e.target.offsetTop;

    
    
    if (Math.abs(this._downX - upX) < MOUSE_CLICK_THRESHOLD &&
        Math.abs(this._downY - upY) < MOUSE_CLICK_THRESHOLD) {

      this.presenter.highlightNodeAt(upX, upY);
    }

    this.arcball.mouseUp(upX, upY, button);
  },

  


  _onMouseMove: function TVC__onMouseMove(e)
  {
    e.preventDefault();
    e.stopPropagation();

    if (this._time < MOUSE_INTRO_DELAY) {
      return;
    }

    
    let moveX = e.clientX - e.target.offsetLeft;
    let moveY = e.clientY - e.target.offsetTop;

    this.arcball.mouseMove(moveX, moveY);
  },

  


  _onMouseOver: function TVC__onMouseOver(e)
  {
    e.preventDefault();
    e.stopPropagation();

    this.arcball.mouseOver();
  },

  


  _onMouseOut: function TVC__onMouseOut(e)
  {
    e.preventDefault();
    e.stopPropagation();

    this.arcball.mouseOut();
  },

  


  _onMozScroll: function TVC__onMozScroll(e)
  {
    e.preventDefault();
    e.stopPropagation();

    this.arcball.zoom(e.detail);
  },

  


  _onKeyDown: function TVC__onKeyDown(e)
  {
    let code = e.keyCode || e.which;

    if (!e.altKey && !e.ctrlKey && !e.metaKey && !e.shiftKey) {
      e.preventDefault();
      e.stopPropagation();
      this.arcball.keyDown(code);
    } else {
      this.arcball.cancelKeyEvents();
    }
  },

  


  _onKeyUp: function TVC__onKeyUp(e)
  {
    let code = e.keyCode || e.which;

    if (code === e.DOM_VK_X) {
      this.presenter.deleteNode();
    }
    if (code === e.DOM_VK_F) {
      let highlight = this.presenter._highlight;
      let zoom = this.presenter.transforms.zoom;

      this.arcball.moveIntoView(vec3.lerp(
        vec3.scale(highlight.v0, zoom, []),
        vec3.scale(highlight.v1, zoom, []), 0.5));
    }
    if (!e.altKey && !e.ctrlKey && !e.metaKey && !e.shiftKey) {
      e.preventDefault();
      e.stopPropagation();
      this.arcball.keyUp(code);
    }
  },

  


  _onKeyPress: function TVC__onKeyPress(e)
  {
    if (e.keyCode === e.DOM_VK_ESCAPE) {
      let mod = {};
      Cu.import("resource:///modules/devtools/Tilt.jsm", mod);
      let tilt =
        mod.TiltManager.getTiltForBrowser(this.presenter.chromeWindow);
      e.preventDefault();
      e.stopPropagation();
      tilt.destroy(tilt.currentWindowId, true);
    }
  },

  


  _onBlur: function TVC__onBlur(e) {
    this.arcball.cancelKeyEvents();
  },

  


  _onResize: function TVC__onResize(e)
  {
    let zoom = this.presenter._getPageZoom();
    let width = e.target.innerWidth * zoom;
    let height = e.target.innerHeight * zoom;

    this.arcball.resize(width, height);
  },

  




  isInitialized: function TVC_isInitialized()
  {
    return this.arcball ? true : false;
  },

  


  _finalize: function TVC__finalize()
  {
    TiltUtils.destroyObject(this.arcball);
    TiltUtils.destroyObject(this._coordinates);

    this.removeEventListeners();
    this.presenter.controller = null;
    this.presenter._controllerUpdate = null;
  }
};



















TiltVisualizer.Arcball = function TV_Arcball(
  aChromeWindow, aWidth, aHeight, aRadius, aInitialTrans, aInitialRot)
{
  


  this.chromeWindow = aChromeWindow;

  


  this._mousePress = vec3.create();
  this._mouseRelease = vec3.create();
  this._mouseMove = vec3.create();
  this._mouseLerp = vec3.create();
  this._mouseButton = -1;

  


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
  this._zoomAmount = 0;

  


  this._additionalRot = vec3.create();
  this._additionalTrans = vec3.create();
  this._deltaAdditionalRot = quat4.create();
  this._deltaAdditionalTrans = vec3.create();

  
  this._loadKeys();

  
  this.resize(aWidth, aHeight, aRadius);
};

TiltVisualizer.Arcball.prototype = {

  









  update: function TVA_update(aDelta)
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

      
      this._pointToSphere(x, y, this.width, this.height, this.radius, endVec);

      
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

    let zoomAmount = this._zoomAmount;
    let keyCode = this._keyCode;

    
    deltaTrans[2] = (zoomAmount - currentTrans[2]) * ARCBALL_ZOOM_STEP;
    currentTrans[2] += deltaTrans[2];

    let additionalRot = this._additionalRot;
    let additionalTrans = this._additionalTrans;
    let deltaAdditionalRot = this._deltaAdditionalRot;
    let deltaAdditionalTrans = this._deltaAdditionalTrans;

    let rotateKeys = this.rotateKeys;
    let panKeys = this.panKeys;
    let zoomKeys = this.zoomKeys;
    let resetKey = this.resetKey;

    
    if (keyCode[rotateKeys.left]) {
      additionalRot[0] -= ARCBALL_SENSITIVITY * ARCBALL_ROTATION_STEP;
    }
    if (keyCode[rotateKeys.right]) {
      additionalRot[0] += ARCBALL_SENSITIVITY * ARCBALL_ROTATION_STEP;
    }
    if (keyCode[rotateKeys.up]) {
      additionalRot[1] += ARCBALL_SENSITIVITY * ARCBALL_ROTATION_STEP;
    }
    if (keyCode[rotateKeys.down]) {
      additionalRot[1] -= ARCBALL_SENSITIVITY * ARCBALL_ROTATION_STEP;
    }
    if (keyCode[panKeys.left]) {
      additionalTrans[0] -= ARCBALL_SENSITIVITY * ARCBALL_TRANSLATION_STEP;
    }
    if (keyCode[panKeys.right]) {
      additionalTrans[0] += ARCBALL_SENSITIVITY * ARCBALL_TRANSLATION_STEP;
    }
    if (keyCode[panKeys.up]) {
      additionalTrans[1] -= ARCBALL_SENSITIVITY * ARCBALL_TRANSLATION_STEP;
    }
    if (keyCode[panKeys.down]) {
      additionalTrans[1] += ARCBALL_SENSITIVITY * ARCBALL_TRANSLATION_STEP;
    }
    if (keyCode[zoomKeys["in"][0]] ||
        keyCode[zoomKeys["in"][1]] ||
        keyCode[zoomKeys["in"][2]]) {
      this.zoom(-ARCBALL_TRANSLATION_STEP);
    }
    if (keyCode[zoomKeys["out"][0]] ||
        keyCode[zoomKeys["out"][1]]) {
      this.zoom(ARCBALL_TRANSLATION_STEP);
    }
    if (keyCode[zoomKeys["unzoom"]]) {
      this._zoomAmount = 0;
    }
    if (keyCode[resetKey]) {
      this.reset();
    }

    
    deltaAdditionalRot[0] +=
      (additionalRot[0] - deltaAdditionalRot[0]) * ARCBALL_SENSITIVITY;
    deltaAdditionalRot[1] +=
      (additionalRot[1] - deltaAdditionalRot[1]) * ARCBALL_SENSITIVITY;
    deltaAdditionalRot[2] +=
      (additionalRot[2] - deltaAdditionalRot[2]) * ARCBALL_SENSITIVITY;

    deltaAdditionalTrans[0] +=
      (additionalTrans[0] - deltaAdditionalTrans[0]) * ARCBALL_SENSITIVITY;
    deltaAdditionalTrans[1] +=
      (additionalTrans[1] - deltaAdditionalTrans[1]) * ARCBALL_SENSITIVITY;

    
    quat4.fromEuler(
      deltaAdditionalRot[0],
      deltaAdditionalRot[1],
      deltaAdditionalRot[2], deltaRot);

    
    vec3.set([deltaAdditionalTrans[0], deltaAdditionalTrans[1], 0], deltaTrans);

    
    if (this._resetInProgress) {
      this._nextResetStep(aDelta || 1);
    }

    
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
    this._cancelReset();
    this._save();

    
    this._pointToSphere(
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

  







  zoom: function TVA_zoom(aZoom)
  {
    this._cancelReset();
    this._zoomAmount = TiltMath.clamp(this._zoomAmount - aZoom,
      ARCBALL_ZOOM_MIN, ARCBALL_ZOOM_MAX);
  },

  






  keyDown: function TVA_keyDown(aCode)
  {
    this._cancelReset();
    this._keyCode[aCode] = true;
  },

  






  keyUp: function TVA_keyUp(aCode)
  {
    this._keyCode[aCode] = false;
  },

  















  _pointToSphere: function TVA__pointToSphere(
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

  


  cancelKeyEvents: function TVA_cancelKeyEvents()
  {
    this._keyCode = {};
  },

  


  cancelMouseEvents: function TVA_cancelMouseEvents()
  {
    this._rotating = false;
    this._mouseButton = -1;
  },

  





  translate: function TVP_translate(aTranslation)
  {
    this._additionalTrans[0] += aTranslation[0];
    this._additionalTrans[1] += aTranslation[1];
  },

  





  rotate: function TVP_rotate(aRotation)
  {
    
    this._additionalRot[0] += TiltMath.radians(aRotation[1]);
    this._additionalRot[1] += TiltMath.radians(aRotation[0]);
    this._additionalRot[2] += TiltMath.radians(aRotation[2]);
  },

  






  moveIntoView: function TVA_moveIntoView(aPoint) {
    let visiblePointX = -(this._currentTrans[0] + this._additionalTrans[0]);
    let visiblePointY = -(this._currentTrans[1] + this._additionalTrans[1]);

    if (aPoint[1] - visiblePointY - MOVE_INTO_VIEW_ACCURACY > this.height ||
        aPoint[1] - visiblePointY + MOVE_INTO_VIEW_ACCURACY < 0 ||
        aPoint[0] - visiblePointX > this.width ||
        aPoint[0] - visiblePointX < 0) {
      this.reset([0, -aPoint[1]]);
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

  







  reset: function TVA_reset(aFinalTranslation, aFinalRotation)
  {
    if ("function" === typeof this._onResetStart) {
      this._onResetStart();
      this._onResetStart = null;
    }

    this.cancelMouseEvents();
    this.cancelKeyEvents();
    this._cancelReset();

    this._save();
    this._resetFinalTranslation = vec3.create(aFinalTranslation);
    this._resetFinalRotation = quat4.create(aFinalRotation);
    this._resetInProgress = true;
  },

  


  _cancelReset: function TVA__cancelReset()
  {
    if (this._resetInProgress) {
      this._resetInProgress = false;
      this._save();

      if ("function" === typeof this._onResetFinish) {
        this._onResetFinish();
        this._onResetFinish = null;
        this._onResetStep = null;
      }
    }
  },

  





  _nextResetStep: function TVA__nextResetStep(aDelta)
  {
    
    
    aDelta = TiltMath.clamp(aDelta, 1, 100);

    let fNearZero = EPSILON * EPSILON;
    let fInterpLin = ARCBALL_RESET_LINEAR_FACTOR * aDelta;
    let fInterpSph = ARCBALL_RESET_SPHERICAL_FACTOR;
    let fTran = this._resetFinalTranslation;
    let fRot = this._resetFinalRotation;

    let t = vec3.create(fTran);
    let r = quat4.multiply(quat4.inverse(quat4.create(this._currentRot)), fRot);

    
    vec3.lerp(this._currentTrans, t, fInterpLin);
    quat4.slerp(this._currentRot, r, fInterpSph);

    
    vec3.scale(this._additionalTrans, fInterpLin);
    vec3.scale(this._additionalRot, fInterpLin);
    this._zoomAmount *= fInterpLin;

    
    if (vec3.length(vec3.subtract(this._lastRot, fRot, [])) < fNearZero &&
        vec3.length(vec3.subtract(this._deltaRot, fRot, [])) < fNearZero &&
        vec3.length(vec3.subtract(this._currentRot, fRot, [])) < fNearZero &&
        vec3.length(vec3.subtract(this._lastTrans, fTran, [])) < fNearZero &&
        vec3.length(vec3.subtract(this._deltaTrans, fTran, [])) < fNearZero &&
        vec3.length(vec3.subtract(this._currentTrans, fTran, [])) < fNearZero &&
        vec3.length(this._additionalRot) < fNearZero &&
        vec3.length(this._additionalTrans) < fNearZero) {

      this._cancelReset();
    }

    if ("function" === typeof this._onResetStep) {
      this._onResetStep();
    }
  },

  


  _loadKeys: function TVA__loadKeys()
  {
    this.rotateKeys = {
      "up": Ci.nsIDOMKeyEvent["DOM_VK_W"],
      "down": Ci.nsIDOMKeyEvent["DOM_VK_S"],
      "left": Ci.nsIDOMKeyEvent["DOM_VK_A"],
      "right": Ci.nsIDOMKeyEvent["DOM_VK_D"],
    };
    this.panKeys = {
      "up": Ci.nsIDOMKeyEvent["DOM_VK_UP"],
      "down": Ci.nsIDOMKeyEvent["DOM_VK_DOWN"],
      "left": Ci.nsIDOMKeyEvent["DOM_VK_LEFT"],
      "right": Ci.nsIDOMKeyEvent["DOM_VK_RIGHT"],
    };
    this.zoomKeys = {
      "in": [
        Ci.nsIDOMKeyEvent["DOM_VK_I"],
        Ci.nsIDOMKeyEvent["DOM_VK_ADD"],
        Ci.nsIDOMKeyEvent["DOM_VK_EQUALS"],
      ],
      "out": [
        Ci.nsIDOMKeyEvent["DOM_VK_O"],
        Ci.nsIDOMKeyEvent["DOM_VK_SUBTRACT"],
      ],
      "unzoom": Ci.nsIDOMKeyEvent["DOM_VK_0"]
    };
    this.resetKey = Ci.nsIDOMKeyEvent["DOM_VK_R"];
  },

  


  _save: function TVA__save()
  {
    if (this._mousePress) {
      let x = this._mousePress[0];
      let y = this._mousePress[1];

      this._mouseMove[0] = x;
      this._mouseMove[1] = y;
      this._mouseRelease[0] = x;
      this._mouseRelease[1] = y;
      this._mouseLerp[0] = x;
      this._mouseLerp[1] = y;
    }
  },

  


  _finalize: function TVA__finalize()
  {
    this._cancelReset();
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

  get introTransition()
  {
    return this._introTransition;
  },

  set introTransition(value)
  {
    TiltUtils.Preferences.set("intro_transition", "boolean", value);
    this._introTransition = value;
  },

  get outroTransition()
  {
    return this._outroTransition;
  },

  set outroTransition(value)
  {
    TiltUtils.Preferences.set("outro_transition", "boolean", value);
    this._outroTransition = value;
  },

  


  load: function TVC_load()
  {
    let prefs = TiltVisualizer.Prefs;
    let get = TiltUtils.Preferences.get;

    prefs._enabled = get("enabled", "boolean");
    prefs._introTransition = get("intro_transition", "boolean");
    prefs._outroTransition = get("outro_transition", "boolean");
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
