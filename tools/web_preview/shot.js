// usage: node shot.js <url> <out.png> <width> <height> [fullPage]
const { chromium } = require(process.env.PW_MODULE || 'playwright-core');
(async () => {
  const [url, out, w, h, full] = process.argv.slice(2);
  const b = await chromium.launch({ executablePath: process.env.CHROME_BIN });
  const p = await b.newPage({ viewport: { width: +w, height: +h }, deviceScaleFactor: 2, isMobile: +w < 600, hasTouch: +w < 600 });
  await p.goto(url); await p.waitForTimeout(2500);
  await p.screenshot({ path: out, fullPage: full === '1' });
  const sw = await p.evaluate(() => document.documentElement.scrollWidth);
  console.log(out, 'scrollWidth', sw);
  await b.close();
})();
