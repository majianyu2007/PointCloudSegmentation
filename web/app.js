import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

const DATASETS = [
  ["data/scene.ply", 16, 3, 30, 10, 0.08, 30],
  ["data/stress_close_parallel_28000.ply", 16, 1, 45, 45, 0.15, 30],
  ["data/stress_complex_35200.ply", 20, 3, 32, 12, 0.10, 80],
  ["data/stress_sparse_noisy_14800.ply", 24, 2, 40, 20, 0.18, 80],
  ["data/stress_large_100000.ply", 24, 3, 32, 14, 0.10, 150],
  ["data/real/stanford_bunny_res3.ply", 12, 2, 35, 18, 0.12, 10],
  ["data/real/stanford_bunny_res2.ply", 16, 2, 35, 18, 0.12, 20],
  ["data/real/stanford_bunny.ply", 16, 3, 28, 8, 0.08, 30],
  ["data/real/stanford_dragon_res3.ply", 18, 3, 35, 16, 0.12, 40],
  ["data/real/stanford_dragon_res2.ply", 16, 3, 28, 8, 0.08, 80],
  ["data/real/stanford_armadillo_50000.ply", 16, 3, 28, 8, 0.08, 50],
];

const $ = (id) => document.getElementById(id);
const ui = Object.fromEntries(["dataset","loadDataset","localFile","run","colorMode","level","levelVal",
  "levelInfo","pointSize","pointSizeVal","bgGray","bgGrayVal","exportName","export","status",
  "front","side","top","k","levels","coarse","fine","curv","minCluster"].map(id => [id, $(id)]));
for (const d of DATASETS) ui.dataset.add(new Option(d[0], d[0]));

let cloud = null, normals = null, curvature = null, segLevels = [], segCounts = [];
let points = null, geometry = null, material = null;
let bbox = new THREE.Box3(), center = new THREE.Vector3(), radius = 1;

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(45, 1, 0.01, 1000);
const renderer = new THREE.WebGLRenderer({ antialias: true });
const controls = new OrbitControls(camera, renderer.domElement);
$("viewer").appendChild(renderer.domElement);
material = new THREE.PointsMaterial({ size: 3, vertexColors: true, sizeAttenuation: false });

function status(s) { ui.status.textContent = "状态: " + s; }
function params() {
  return { k:+ui.k.value, levels:+ui.levels.value, coarse:+ui.coarse.value,
    fine:+ui.fine.value, curv:+ui.curv.value, min:+ui.minCluster.value };
}
function applyPreset(path) {
  const d = DATASETS.find(x => x[0] === path) || DATASETS[0];
  [ui.k.value, ui.levels.value, ui.coarse.value, ui.fine.value, ui.curv.value, ui.minCluster.value] =
    [d[1], d[2], d[3], d[4], d[5], d[6]];
  syncLabels();
}
function syncLabels() {
  for (const id of ["k","levels","coarse","fine","curv","minCluster","pointSize","bgGray"])
    $(id + "Val").textContent = (+ui[id].value).toFixed(id === "curv" || id === "bgGray" ? 3 : id.includes("coarse") || id.includes("fine") || id === "pointSize" ? 1 : 0);
  material.size = +ui.pointSize.value;
  renderer.setClearColor(new THREE.Color(+ui.bgGray.value, +ui.bgGray.value, +ui.bgGray.value));
}

function parsePLY(text) {
  const lines = text.split(/\r?\n/);
  let count = 0, props = [], i = 0, inVertex = false;
  for (; i < lines.length; ++i) {
    const p = lines[i].trim().split(/\s+/);
    if (p[0] === "element") { inVertex = p[1] === "vertex"; if (inVertex) count = +p[2]; }
    else if (p[0] === "property" && inVertex) props.push(p[p.length - 1]);
    else if (p[0] === "end_header") { ++i; break; }
  }
  const ix = props.indexOf("x"), iy = props.indexOf("y"), iz = props.indexOf("z");
  const pos = new Float32Array(count * 3);
  for (let v = 0; v < count && i < lines.length; ++v, ++i) {
    const a = lines[i].trim().split(/\s+/).map(Number);
    pos[v*3] = a[ix]; pos[v*3+1] = a[iy]; pos[v*3+2] = a[iz];
  }
  return { positions: pos, count };
}

function setCloud(c) {
  cloud = c; normals = curvature = null; segLevels = []; segCounts = [];
  if (points) scene.remove(points);
  geometry = new THREE.BufferGeometry();
  geometry.setAttribute("position", new THREE.BufferAttribute(cloud.positions, 3));
  geometry.setAttribute("color", new THREE.BufferAttribute(new Float32Array(cloud.count * 3), 3));
  points = new THREE.Points(geometry, material);
  scene.add(points);
  bbox.setFromBufferAttribute(geometry.getAttribute("position"));
  bbox.getCenter(center); radius = bbox.getSize(new THREE.Vector3()).length() * 0.5 || 1;
  controls.target.copy(center);
  camera.position.set(center.x, center.y - radius * 2.5, center.z + radius * 1.4);
  camera.near = Math.max(0.001, radius * 0.01); camera.far = radius * 100; camera.updateProjectionMatrix();
  colorize();
  status(`已载入点云，点数 ${cloud.count}`);
}

class KdNode {
  constructor(items, depth, pos) {
    this.axis = depth % 3;
    items.sort((a,b) => pos[a*3+this.axis] - pos[b*3+this.axis]);
    const m = items.length >> 1;
    this.index = items[m];
    this.left = m ? new KdNode(items.slice(0, m), depth + 1, pos) : null;
    this.right = m + 1 < items.length ? new KdNode(items.slice(m + 1), depth + 1, pos) : null;
  }
}
function dist2(pos, i, qx, qy, qz) {
  const dx = pos[i*3]-qx, dy = pos[i*3+1]-qy, dz = pos[i*3+2]-qz;
  return dx*dx + dy*dy + dz*dz;
}
function kNearest(root, pos, qi, k) {
  const qx = pos[qi*3], qy = pos[qi*3+1], qz = pos[qi*3+2], best = [];
  function add(i, d) { best.push([d, i]); best.sort((a,b)=>b[0]-a[0]); if (best.length > k) best.shift(); }
  function visit(n) {
    if (!n) return;
    add(n.index, dist2(pos, n.index, qx, qy, qz));
    const diff = [qx, qy, qz][n.axis] - pos[n.index*3+n.axis];
    const near = diff < 0 ? n.left : n.right, far = diff < 0 ? n.right : n.left;
    visit(near);
    if (best.length < k || diff*diff < best[0][0]) visit(far);
  }
  visit(root);
  return best.map(x => x[1]);
}

function smallestEigenVector(c) {
  let a = [[c[0],c[1],c[2]],[c[1],c[3],c[4]],[c[2],c[4],c[5]]], v = [[1,0,0],[0,1,0],[0,0,1]];
  for (let it = 0; it < 10; ++it) {
    let p = 0, q = 1, max = Math.abs(a[0][1]);
    for (const [i,j] of [[0,2],[1,2]]) if (Math.abs(a[i][j]) > max) { max = Math.abs(a[i][j]); p = i; q = j; }
    if (max < 1e-8) break;
    const t = 0.5 * Math.atan2(2*a[p][q], a[q][q]-a[p][p]), ct = Math.cos(t), st = Math.sin(t);
    for (let k = 0; k < 3; ++k) { const apk = a[p][k], aqk = a[q][k]; a[p][k]=ct*apk-st*aqk; a[q][k]=st*apk+ct*aqk; }
    for (let k = 0; k < 3; ++k) { const akp = a[k][p], akq = a[k][q]; a[k][p]=ct*akp-st*akq; a[k][q]=st*akp+ct*akq; }
    for (let k = 0; k < 3; ++k) { const vkp = v[k][p], vkq = v[k][q]; v[k][p]=ct*vkp-st*vkq; v[k][q]=st*vkp+ct*vkq; }
  }
  let m = a[0][0] < a[1][1] ? 0 : 1; if (a[2][2] < a[m][m]) m = 2;
  const n = [v[0][m], v[1][m], v[2][m]], len = Math.hypot(...n) || 1;
  return [n[0]/len, n[1]/len, n[2]/len, Math.max(0, a[m][m])];
}

async function estimateAndSegment() {
  if (!cloud) return;
  const p = params(), n = cloud.count, pos = cloud.positions;
  status("建立 KD 树...");
  const root = new KdNode([...Array(n).keys()], 0, pos);
  const adj = Array(n);
  normals = new Float32Array(n * 3); curvature = new Float32Array(n);
  status("估计法向量...");
  for (let i = 0; i < n; ++i) {
    const nb = kNearest(root, pos, i, p.k); adj[i] = nb;
    let mx=0,my=0,mz=0; for (const j of nb) { mx+=pos[j*3]; my+=pos[j*3+1]; mz+=pos[j*3+2]; }
    mx/=nb.length; my/=nb.length; mz/=nb.length;
    const cov = [0,0,0,0,0,0];
    for (const j of nb) { const x=pos[j*3]-mx,y=pos[j*3+1]-my,z=pos[j*3+2]-mz; cov[0]+=x*x; cov[1]+=x*y; cov[2]+=x*z; cov[3]+=y*y; cov[4]+=y*z; cov[5]+=z*z; }
    const e = smallestEigenVector(cov); normals[i*3]=e[0]; normals[i*3+1]=e[1]; normals[i*3+2]=e[2];
    const sum = cov[0]+cov[3]+cov[5]; curvature[i] = sum > 1e-8 ? e[3] / sum : 0;
    if (i % 2500 === 0) await new Promise(requestAnimationFrame);
  }
  status("区域生长分割...");
  segment(adj, p);
  ui.level.max = String(segLevels.length - 1); ui.level.value = String(segLevels.length - 1);
  colorize();
  status(`分割完成，各层段数: ${segCounts.map((c,i)=>`L${i}=${c}`).join(" ")}`);
}

function grow(adj, allowed, angleDeg, curv, minSize) {
  const n = cloud.count, labels = new Int32Array(n); labels.fill(-1);
  const order = [...Array(n).keys()].filter(i => allowed[i]).sort((a,b)=>curvature[a]-curvature[b]);
  const cosT = Math.cos(angleDeg * Math.PI / 180), counts = [];
  let sid = 0;
  for (const seed of order) {
    if (labels[seed] !== -1) continue;
    const q = [seed]; labels[seed] = sid; let head = 0, count = 1;
    while (head < q.length) {
      const cur = q[head++], nx=normals[cur*3], ny=normals[cur*3+1], nz=normals[cur*3+2];
      for (const nb of adj[cur]) if (allowed[nb] && labels[nb] === -1) {
        const d = Math.abs(nx*normals[nb*3] + ny*normals[nb*3+1] + nz*normals[nb*3+2]);
        if (d < cosT) continue;
        labels[nb] = sid; ++count; if (curvature[nb] < curv) q.push(nb);
      }
    }
    counts[sid++] = count;
  }
  const remap = new Int32Array(sid).fill(-1); let kept = 0;
  for (let s = 0; s < sid; ++s) if (counts[s] >= minSize) remap[s] = kept++;
  for (let i = 0; i < n; ++i) if (labels[i] >= 0) labels[i] = remap[labels[i]];
  return [labels, kept];
}
function segment(adj, p) {
  const n = cloud.count; segLevels = []; segCounts = [];
  let [labels, count] = grow(adj, new Uint8Array(n).fill(1), p.coarse, p.curv, p.min);
  segLevels.push(labels); segCounts.push(count);
  for (let L = 1; L < p.levels; ++L) {
    const deg = p.coarse + L/(p.levels-1) * (p.fine-p.coarse), out = new Int32Array(n).fill(-1);
    let global = 0, prev = segLevels[segLevels.length - 1];
    for (let s = 0; s < count; ++s) {
      const allowed = new Uint8Array(n); for (let i = 0; i < n; ++i) if (prev[i] === s) allowed[i] = 1;
      const [sub, subCount] = grow(adj, allowed, deg, p.curv, p.min);
      for (let i = 0; i < n; ++i) if (sub[i] >= 0) out[i] = global + sub[i];
      global += subCount;
    }
    labels = out; count = global; segLevels.push(labels); segCounts.push(count);
  }
}

function labelColor(label) {
  if (label < 0) return [0.5,0.5,0.5];
  const h = (label * 137.508 % 360) / 60, s = 0.65, v = 0.95, i = Math.floor(h), f = h - i;
  const p = v*(1-s), q = v*(1-s*f), t = v*(1-s*(1-f));
  return [[v,t,p],[q,v,p],[p,v,t],[p,q,v],[t,p,v],[v,p,q]][i % 6];
}
function colorize() {
  if (!cloud || !geometry) return;
  const mode = ui.colorMode.value, level = Math.min(+ui.level.value, segLevels.length - 1), col = geometry.getAttribute("color").array;
  let zmin = Infinity, zmax = -Infinity;
  for (let i = 0; i < cloud.count; ++i) { const z = cloud.positions[i*3+2]; zmin = Math.min(zmin,z); zmax = Math.max(zmax,z); }
  const zr = zmax > zmin ? zmax - zmin : 1;
  for (let i = 0; i < cloud.count; ++i) {
    let c = [0.75,0.78,0.82];
    if (mode === "segment" && segLevels[level]) c = labelColor(segLevels[level][i]);
    else if (mode === "normal" && normals) c = [(normals[i*3]+1)/2, (normals[i*3+1]+1)/2, (normals[i*3+2]+1)/2];
    else if (mode === "height") { const t = (cloud.positions[i*3+2]-zmin)/zr; c = [t,0.4,1-t]; }
    col[i*3]=c[0]; col[i*3+1]=c[1]; col[i*3+2]=c[2];
  }
  geometry.getAttribute("color").needsUpdate = true;
  ui.levelVal.textContent = String(level);
  ui.levelInfo.textContent = `第 ${level} 层，段数 = ${segCounts[level] || 0}`;
}

function exportPLY() {
  if (!cloud || !segLevels.length) return status("导出失败: 没有可导出的分割结果");
  const level = Math.min(+ui.level.value, segLevels.length - 1);
  let s = `ply\nformat ascii 1.0\nelement vertex ${cloud.count}\nproperty float x\nproperty float y\nproperty float z\nproperty uchar red\nproperty uchar green\nproperty uchar blue\nend_header\n`;
  for (let i = 0; i < cloud.count; ++i) {
    const c = labelColor(segLevels[level][i]).map(x => Math.round(x * 255));
    s += `${cloud.positions[i*3]} ${cloud.positions[i*3+1]} ${cloud.positions[i*3+2]} ${c[0]} ${c[1]} ${c[2]}\n`;
  }
  const a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob([s], { type: "text/plain" }));
  a.download = ui.exportName.value || "segmented_web.ply"; a.click(); URL.revokeObjectURL(a.href);
}

async function loadPath(path) {
  applyPreset(path); status("读取点云文件...");
  const text = await (await fetch(path)).text();
  setCloud(parsePLY(text));
}
ui.loadDataset.onclick = () => loadPath(ui.dataset.value).catch(e => status("载入失败: " + e.message));
ui.dataset.onchange = () => applyPreset(ui.dataset.value);
ui.localFile.onchange = async () => { const f = ui.localFile.files[0]; if (f) setCloud(parsePLY(await f.text())); };
ui.run.onclick = () => estimateAndSegment();
ui.export.onclick = exportPLY;
ui.colorMode.onchange = ui.level.oninput = colorize;
for (const id of ["k","levels","coarse","fine","curv","minCluster","pointSize","bgGray"]) ui[id].oninput = syncLabels;
ui.front.onclick = () => { camera.position.set(center.x, center.y - radius*3, center.z); controls.update(); };
ui.side.onclick = () => { camera.position.set(center.x + radius*3, center.y, center.z); controls.update(); };
ui.top.onclick = () => { camera.position.set(center.x, center.y, center.z + radius*3); controls.update(); };
function resize() {
  const el = $("viewer"), w = el.clientWidth, h = el.clientHeight;
  renderer.setSize(w, h, false); camera.aspect = w / h; camera.updateProjectionMatrix();
}
function animate() { resize(); controls.update(); renderer.render(scene, camera); requestAnimationFrame(animate); }
applyPreset(DATASETS[0][0]); syncLabels(); animate(); loadPath(DATASETS[0][0]);
