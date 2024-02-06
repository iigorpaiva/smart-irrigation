import React, { Component } from 'react';
import MoistureSensor from './components/moisture-sensor/MoistureSensor';
import "./style.css"
import Card from './shared/Card/Card';
import Logo from './components/logo/logo';
import IrrigationMode from './components/irrigation-mode/irrigation-mode';
import TimePickerComponent from './components/time-picker/time-picker';

class App extends Component {
  constructor(props) {
    super(props);
    this.state = { ledOn: false, updating: false, sensorData: null };

    // Bind the methods to the current instance of the class
    this.setLedState = this.setLedState.bind(this);
    this.handleStateChange = this.handleStateChange.bind(this);
    this.fetchMoistureData = this.fetchMoistureData.bind(this);
    this.checkLedStatus = this.checkLedStatus.bind(this);
  }

  setLedState(state) {
    this.setState({ ledOn: state !== '0', updating: false });
  }

  handleStateChange(ledOn) {
    this.setState({ updating: true });

    fetch('/led', { method: 'PUT', body: ledOn ? '0' : '1', timeout: 1000 })
      .then(response => response.text())
      .then(state => this.setLedState(state))
      .catch(error => {
        console.error('Error in request:', error);
        this.setState({ updating: false });
      });
  }

  fetchMoistureData = async () => {
    try {
      const response = await fetch('/moisture'); 
      const sensorData = await response.json();
      this.setState({ sensorData });
    } catch (error) {
      console.error('Error fetching sensor data:', error);
    }
  };

  checkLedStatus = () => {
    fetch('/led')
      .then(response => response.text())
      .then(state => this.setLedState(state))
      .catch(error => {
        console.error('Error checking LED status:', error);
      });
  };

  componentDidMount() {
    this.checkLedStatus();
    this.fetchMoistureData();
    this.intervalID = setInterval(() => {
      this.checkLedStatus();
      this.fetchMoistureData();
    }, 1000);
  }

  componentWillUnmount() {
    clearInterval(this.intervalID);
  }

  render() {
    const { ledOn, sensorData } = this.state;

    return (
      <div>
        <div className="main">
          <div className="container row">
            <div className="col-md-12">
              <Card title="Agendamento" body={<TimePickerComponent />} />
            </div>
            <div className="col-md-12">
              <Card title="Umidade do Solo" body={<MoistureSensor sensorData={sensorData} />} />
            </div>
            <div className="col-md-12">
              <Card title="Modo Irrigação" body={<IrrigationMode value={ledOn} onToggle={value => this.handleStateChange(value)} disabled={this.state.updating} />} />
            </div>
          </div>
        </div>
      </div>
    );
  }
}

export default App;
