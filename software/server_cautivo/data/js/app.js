Vue.use(VueNativeSock.default, 'ws://' + location.hostname + '/ws', { format: 'json' })

Vue.component('gpio-output', {
  props: ['gpio'],
  template:   //HTML
   ` 
    <v-list-tile avatar>
    
      <v-list-tile-content>
        <v-list-tile-title>{{gpio.text}}</v-list-tile-title>
      </v-list-tile-content>

      <v-list-tile-action>
        <v-switch v-model="gpio.status" :label="gpio.status ? 'ON' : 'OFF'" @change="sendGPIO"></v-switch>
      </v-list-tile-action>

      <v-list-tile-action>        
        <v-icon :color="gpio.ack ? 'green' : 'grey lighten-3'" >fiber_manual_record</v-icon>
      </v-list-tile-action>
            
    </v-list-tile>
  `,
  methods: {
    sendGPIO: function (evt) {
      this.gpio.ack = 0;      
      console.log(this.gpio.id + '-' + this.gpio.text + ': ' + this.gpio.status);

      let data = {
        command: "setGPIO",
        id: this.gpio.id,
        text: this.gpio.text,
        status: this.gpio.status
      }

      let json = JSON.stringify(data);
      console.log(json);
      this.$socket.send(json);
    }
  }
})


Vue.component('action', {
  props: ['action'],
  template:   //HTML
   ` 
    <v-list-tile avatar>    
      <v-list-tile-action>
        <v-btn text small class="ma-2" @click="doAction">{{action.text_btn}}</v-btn>
      </v-list-tile-action> 

      <v-list-tile-content>
        <v-list-tile-title v-if="action.status">{{action.text}}</v-list-tile-title>
      </v-list-tile-content>
    </v-list-tile>
  `,
  methods: {
    doAction: function (evt) {
      this.action.status = !(this.action.status);
      console.log(this.action.text);
      let data = {
        command: "doAction",
        text: this.action.text_btn,
        id: this.action.id,
        status: this.action.status
      }

      let json = JSON.stringify(data);
      console.log(json);
      this.$socket.send(json);

      this.action.callback();
      
      if(this.action.id == 1) {
        // setTimeout: llama a callback luego de la espera en milisegundos
        setTimeout(() => {window.location.reload()},7000);          
      }
      //if(this.action.id == 0) {
      //  setTimeout(() => {this.action.status = 0},5000);          
      //}
            
    }
  }
})

Vue.component('lora_cfg', {
  props: ['lora_cfg'],
  template:   //HTML
   `     
    <v-list-tile>
      
      <v-list-tile-title style="width: 128px">{{lora_cfg.text}}</v-list-tile-title>
      
      <v-list-tile-action>
        <v-slider thumb-label v-model="lora_cfg.value" :min="lora_cfg.min" :max="lora_cfg.max" @change="sendConfig">
          <template v-slot:append>
            <v-text-field class="mt-0 pt-0" hide-details single-line type="number" style="width: 38px"
              v-model="lora_cfg.value" :min="lora_cfg.min" :max="lora_cfg.max" @change="sendConfig">
            </v-text-field>
          </template>
        </v-slider>
      </v-list-tile-action>     
   
    </v-list-tile>
    `,
  methods: {
    sendConfig: function (evt) {
      console.log(this.lora_cfg.text + ': ' + this.lora_cfg.value);

      let data = {
        command: this.lora_cfg.cfg,
        id: this.lora_cfg.id,
        value: this.lora_cfg.value
      }

      let json = JSON.stringify(data);
      this.$socket.send(json);
    }   
  }
})

var app = new Vue({
  el: '#app',
  data: function () {
    return {
      gpio_output_list: [
        { id: 0, text: 'P1', status: 0, ack: 0 },
        { id: 1, text: 'P3', status: 0, ack: 0 },
        { id: 2, text: 'P4', status: 0, ack: 0 },
        { id: 3, text: 'P6', status: 0, ack: 0 }
      ],
      action_list: [
        { id: 0, status: 0, text: 'Modulación Lenta', text_btn: 'Modulación', callback: () => console.log("Modulación") },
        { id: 1, status: 0, text: 'reiniciando servidor...', text_btn: 'Reset', callback: () => console.log("Reset ESP") }        
      ],
      lora_cfg_list: [
        { id: 0, text: 'Potencia',   cfg: 'txpower', value: 20, min: 5, max: 20},
        { id: 1, text: 'Reintentos', cfg: 'retries', value: 3,  min: 3, max: 6 },
        { id: 2, text: 'Timeout',    cfg: 'timeout', value: 4,  min: 4, max: 8 }
      ],
      rssi: '?',
      signal: '?',
      ssid: '',
      ip: '',
      channel: '',
      mac: '',
      drawer: false,
      version: 'v1.0'
    }
  },
  mounted() {    
    this.$socket.onmessage = (dr) => {
      //console.log(dr);
      let json = JSON.parse(dr.data);
      console.log(json);

      if(json.command == 'wifi_info') {
        this.signal = json.signal;
        if(json.ssid) {
          this.ssid = json.ssid;
          this.ip = json.ip;
          this.channel = json.channel;        
          this.mac = json.mac;        
          this.$data.lora_cfg_list[0].value = json.txpower;       
          this.$data.lora_cfg_list[1].value = json.retries;        
          this.$data.lora_cfg_list[2].value = json.timeout;
          this.$data.action_list[0].status = json.slow;
        }
      }
      else if(json.command == 'doAction') {
        this.$data.action_list[json.id].status = json.status
      }
      else {
        let gpio = this.$data.gpio_output_list.find(gpio => gpio.id == json.id);
        gpio.status = json.status;
        gpio.ack = json.ack;
        this.rssi = json.rssi; 
      }
      
    }
  }
})