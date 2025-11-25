const version = 'uli78-v1'
const assets = [
	'index.html',
	'uli78.js',
	'uli78.wasm',
	'uli78-180.png',
	'uli78-192.png',
	'uli78-512.png',
	'serviceworker.js',
	'uli78.webmanifest'
]

self.addEventListener('install', function(event) {
  console.log('serviceworker installing')
  caches.open(version)
    .then(function(cache) {
      return cache.addAll(assets);
    })
});

self.addEventListener('fetch', function(event) {
  event.respondWith(
    caches.match(event.request)
      .then(function(response) {
        if (response) {
          return response;
        }
        return fetch(event.request);
      }
    )
  );
});

