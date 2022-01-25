var currentHandle = null;
var currentField = null;

const hsv2hsl = (h, s, v, 	l = v - v * s / 2, m = Math.min(l, 1 - l)) => ({h: h, s: m ? (v - l) / m : 0, l: l})
const hsl2hsv = (h, s, l,	v = s * Math.min(l, 1 - l) + l) => ({h: h, s: (v ? 2 - 2 * l / v : 0), v: v});
const clamp = (v, l, h) => Math.max(l, Math.min(v, h))

const _ = (cs, src = document) => (src.querySelector(cs))
const _all = (cs, src = document) => (src.querySelectorAll(cs))
const _forAll = (cs, fun, src = document) => (Object.entries(_all(cs, src)).forEach(([, e]) => fun(e)))
const _csset = (n, v, src) => (src.style.setProperty(n, v))
const _csget = (n, src) => (src.style.getPropertyValue(n))

const lum = hsl => {
	let f = (n, k = (n + hsl.h / 30) % 12) => hsl.l - hsl.s * Math.min(hsl.l, 1 - hsl.l) * Math.max(Math.min(k - 3, 9 - k, 1), -1);
	let rln = c => c <= 0.03928 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4
	return 0.2126 * rln(f(0)) + 0.7152 * rln(f(8)) + 0.0722 * rln(f(4))
}

const ctCol = hsl => {
	let l = (a, b) => (a + 0.05) / (b + 0.05)
	let hsll = lum(hsl)
	return l(1, hsll) > l(hsll, 0) ? "#fff" : "#000"
}

const hsl2css = (hsl) => `hsl(${hsl.h}, ${hsl.s * 100}%, ${hsl.l * 100}%)`
const css2hsl = (css) => {
	let m = css.match(/hsl\s*\(\s*(\d+(?:\.\d+)?)\s*,\s*(\d+(?:\.\d+)?)%\s*,\s*(\d+(?:\.\d+)?)%\s*\)/i)
	if (m) return {h: m[1], s: m[2] / 100, l: m[3] / 100}
	return {h: 0, s: 0, l: 0}
}

const getCssHsl = src => ({h: Number(_csget("--hue", src)), s: Number(_csget("--sat", src)), l: Number(_csget("--lig", src))})

function setNewColorHSL() {
	let p = _(".color-field.show")
	let hsl = getCssHsl(p)
	_(".color-new", p).style.display = ""
	_(".color-picker .color-text", p).innerText = `HSL ${Math.round(hsl.h)} ${Math.round(hsl.s * 100)}% ${Math.round(hsl.l * 100)}%`
	_csset("--text-contrast-color", ctCol(hsl), p)
}

function exitPicker(apply = true) {
	if (apply) {
		_csset("--selected-color", hsl2css(getCssHsl(currentField)), currentField)
	}
	currentField.closest(".color-field").classList.remove("show")
	currentField = null;
}

function set_hue_sat_selector() {
	let h = _(".color-hue", currentField), 
		s = _(".color-saturation", currentField)
		hs = _(".selector", h),
		ss = _(".selector", s),
		hsl = getCssHsl(currentField),
		hsv = hsl2hsv(hsl.h, hsl.s, hsl.l),
		sw = s.scrollWidth, sh = s.scrollHeight,
		ssw = ss.scrollWidth, ssh = ss.scrollHeight
	hs.style.transform = `translate(0, ${h.clientHeight * hsl.h / 360}px)`
	ss.style.transform = `translate(${(sw - ssw - 4) * hsv.s + (ssw / 2 + 2)}px, ${(sh - ssh - 4) * (1 - hsv.v) + (ssh / 2 + 2)}px)`
}

function color_saturation_move(t, s, cP, rP) {
	s.style.transform = `translate(${cP.x}px, ${cP.y}px)`;
	let hsl = hsv2hsl(0, rP.x, 1 - rP.y);
	_csset("--sat", hsl.s, currentField)
	_csset("--lig", hsl.l, currentField)
	setNewColorHSL();
}

function color_hue_move(t, s, cP, rP) {
	s.style.transform = `translate(0, ${cP.y}px)`
	_csset("--hue", 360 * rP.y, currentField)
	setNewColorHSL()
}

function color_picker_mousemove(e, t) {
	let s = _(".selector", t),
		tR = t.getBoundingClientRect(),
		min = {x: 0, y: 0}, max = {x: 0, y: 0}

	const cP = () => ({
		x: clamp(e.pageX - (tR.left + window.scrollX), min.x, max.x),
		y: clamp(e.pageY - (tR.top + window.scrollY), min.y, max.y)
	});

	const rP = () => ({
		x: (cP().x - min.x) / (max.x - min.x),
		y: (cP().y - min.y) / (max.y - min.y)
	});
	
	if (t.classList.contains("color-saturation")) {
		min = {
			x: s.scrollWidth / 2 + 2,
			y: s.scrollHeight / 2 + 2
		}
		max = {
			x: t.scrollWidth - min.x,
			y: t.scrollHeight - min.y
		}
		color_saturation_move(t, s, cP(), rP());
	}
	else if (t.classList.contains("color-hue")) {
		max = {
			x: 1, // Bc 0 is not allowed, this Value will never be used
			y: t.clientHeight - s.scrollHeight + 6
		}
		color_hue_move(t, s, cP(), rP());
	}
}	

_forAll(".color-saturation", e => {
	e.addEventListener("mousedown", (ev) => {
		if (ev.button == 0) {
			currentHandle = ev.target
			color_picker_mousemove(ev, ev.target)
		}	
	})
	e.addEventListener("touchstart", (ev) => {
		currentHandle = ev.target
		color_picker_mousemove(ev, ev.target)
	})
})

_forAll(".color-hue", e => {
	e.addEventListener("mousedown", (ev) => {
		if (ev.button == 0) {
			currentHandle = ev.target
			color_picker_mousemove(ev, ev.target)
		}	
	})
	e.addEventListener("touchstart", (ev) => {
		currentHandle = ev.target
		color_picker_mousemove(ev, ev.target)
	})
})

let popupButtons = Object.entries(_all(".color-field")).map(v => v[1])

window.addEventListener("click", ({target}) => {
	if (target.closest(".color-picker") != null) return;
	if (currentHandle != null) return;
	let pb = target.closest(".color-field");
	let popupToShow = pb && !pb.classList.contains("show");

	popupButtons.forEach(p => p.classList.remove("show"));
	if (currentField) exitPicker()

	if (popupToShow) {
		currentField = pb;
		let hsl = css2hsl(_csget("--selected-color", pb))
		_csset("--hue", hsl.h, pb)
		_csset("--sat", hsl.s, pb)
		_csset("--lig", hsl.l, pb)
		set_hue_sat_selector()
		pb.classList.add("show");
	}
});

window.addEventListener("mousemove", (event) => {
	if ((event.buttons & 1) != 1) currentHandle = null;
	if (currentHandle != null) {
		color_picker_mousemove(event, currentHandle);
	}
})

window.addEventListener("touchstart", ({target}) => {
	if (target.closest(".color-picker") != null) return;
	let pb = target.closest(".color-field");
	let popupToShow = pb && !pb.classList.contains("show");

	popupButtons.forEach(p => p.classList.remove("show"));
	if (currentField) exitPicker()

	if (popupToShow) {
		currentField = pb;
		let hsl = css2hsl(_csget("--selected-color", pb))
		_csset("--hue", hsl.h, pb)
		_csset("--sat", hsl.s, pb)
		_csset("--lig", hsl.l, pb)
		set_hue_sat_selector()
		pb.classList.add("show");
	}
});

window.addEventListener("touchmove", (event) => {
	if (currentHandle != null) {
		color_picker_mousemove(event, currentHandle);
	}
})

window.addEventListener("touchend", (event) => {
	currentHandle = null;
})

window.addEventListener("touchcancel", (event) => {
	currentHandle = null;
})
