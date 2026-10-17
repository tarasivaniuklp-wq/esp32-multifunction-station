.data
.section .rodata.embedded

.global _binary_index_html_start
_binary_index_html_start:
    .incbin "src/index.html"
.global _binary_index_html_end
_binary_index_html_end:

.global _binary_telemetry_html_start
_binary_telemetry_html_start:
    .incbin "src/telemetry.html"
.global _binary_telemetry_html_end
_binary_telemetry_html_end:

.global _binary_settings_html_start
_binary_settings_html_start:
    .incbin "src/settings.html"
.global _binary_settings_html_end
_binary_settings_html_end: