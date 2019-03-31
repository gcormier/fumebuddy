static const char AUX_espurna_settings[] PROGMEM = R"raw(
[
  {
    "title": "FumeBuddy Settings",
    "uri": "/espurna_setting",
    "menu": true,
    "element": [
      {
        "name": "header",
        "type": "ACText",
        "value": "<h2>FumeBuddy settings</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "caption",
        "type": "ACText",
        "value": "Enter your values from the ESPURNA Admin page. Make sure \"Restful API\" is disabled!",
        "style": "font-family:serif;color:#4682b4;"
      },
      {
        "name": "espurnaserver",
        "type": "ACInput",
        "value": "",
        "label": "IP Address",
        "placeholder": "espurna device IP or hostname"
      },
      {
        "name": "apikey",
        "type": "ACInput",
        "label": "API Key"
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save",
        "uri": "/espurna_save"
      }
    ]
  },
  {
    "title": "espurna Settings",
    "uri": "/espurna_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parameters",
        "type": "ACText"
      }
    ]
  }
]
)raw";
