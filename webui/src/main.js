import App from './App.svelte';

const app = new App({
	target: document.body,
	props: {
		title: 'Esp8266 WebAdmin'
	}
});

export default app;
