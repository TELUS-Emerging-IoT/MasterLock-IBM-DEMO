import React, { Component } from 'react';
import { AppLoading, Asset} from 'expo';
import { Alert, AppRegistry, Button, Image, Platform, StyleSheet, Text, TouchableHighlight,
  TouchableOpacity, TouchableNativeFeedback, TouchableWithoutFeedback, View} from 'react-native';

//define images
const availableImg = require("./images/unlockedNew-min.png");
const unavailableImg = require("./images/lockedNew-min.png");
const headerImg = require("./images/header.png");

//define HTTP request timeout
const FETCH_TIMEOUT = 500;
let didTimeOut = false;

//define GET requests
const myUnlockRequest = new Request("http://masterlock-calgarydemo2.mybluemix.net/request?payload=unlock&topic=");
const myLockRequest = new Request("http://masterlock-calgarydemo2.mybluemix.net/request?payload=lock&topic=");

function cacheImages(images) {
  return images.map(image => {
    if (typeof image === 'string') {
      return Image.prefetch(image);
    } else {
      return Asset.fromModule(image).downloadAsync();
    }
  });
}

function timeout(ms, promise) {
  return new Promise(function(resolve, reject) {
    setTimeout(function() {
      reject(new Error("timeout"))
    }, ms)
    promise.then(resolve, reject)
  });
}

export default class MasterLockApp extends Component {
  //constructor function for setting state of image
  constructor() {
    super();
    this.state = {showAvailable: false};
    this._lock = this._lock.bind(this);
    this._unlock = this._unlock.bind(this);
  }

  async _loadAssetsAsync() {
    const imageAssets = cacheImages([
      availableImg, unavailableImg
    ]);
    await Promise.all([...imageAssets]);
  }

  /**button handler functions**/
  //lock function
  _lock() {
    timeout(1000, fetch(myLockRequest))
    .then(response => {
      if (response.status == 200) {
        this.setState({showAvailable: false});
        Alert.alert('Facial unlock is now disabled.');
      }
      else {
        Alert.alert('Failed to bring system offline!');
        throw new Error('Something went wrong!');
      }
    }).catch(function(error){
      Alert.alert('Failed to bring system offline!');
      throw new Error('Something went wrong!');
    });
  }

  //unlock function
  _unlock() {
    timeout(1000, fetch(myUnlockRequest))
    .then(response => {
      if (response.status == 200) {
        this.setState({showAvailable: true});
        Alert.alert('Facial unlock is now available.');
      }
      else {
        Alert.alert('Failed to bring system online!');
        throw new Error('Something went wrong!');
      }
    }).catch(function(error){
      Alert.alert('Failed to bring system offline!');
      throw new Error('Something went wrong!');
    });
  }


  /**display UI elements**/
  render() {
    if (!this.state.isReady) {
      return (
        <AppLoading
          startAsync={this._loadAssetsAsync}
          onFinish={() => this.setState({ isReady: true })}
          onError={console.warn}
        />
      );
    }
    return (
      <View style={styles.container}>
        //header banner
        <View style={styles.headerContainer}>
          <Image
            style={styles.imageHeader}
            source = {headerImg}
          />
        </View>

        //buttons
        <View style={styles.buttonContainer}>
          <Text style={styles.introText}>This is a demo app for the Telus IoT Lock.</Text>

          //unlock button
          <TouchableHighlight style={styles.buttonOnline} onPress={this._unlock} underlayColor="white">
            <Text style={styles.buttonOnlineText}>Make Available</Text>
          </TouchableHighlight>

          //lock button
          <TouchableHighlight style={styles.buttonOffline} onPress={this._lock} underlayColor="white">
            <Text style={styles.buttonOfflineText}>Lock Down</Text>
          </TouchableHighlight>
        </View>

        //image
        <View style={styles.imgContainer}>
          <Image
            style={styles.imageLock}
              source={this.state.showAvailable? availableImg : unavailableImg}
          />
          {this.state.showAvailable?
            <Text style={styles.endText}>The system is currently available for unlocking.</Text>
            : <Text style={styles.endText}>The system is currently locked.</Text>}
        </View>
      </View>
    );
  }
}

//style definitions
const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    height:1000,
    backgroundColor: '#FFFFFF'
  },
  headerContainer: {
    paddingTop: 35,
    width: 500,
    height: 100,
    alignItems: 'center',
    backgroundColor: '#FFFFFF'
  },
  buttonContainer: {
    paddingTop: 20,
    width: 500,
    height: 200,
    alignItems: 'center',
    backgroundColor: '#F1F1F1'
  },
  buttonOffline: {
    marginBottom: 20,
    width: 250,
    height: 50,
    alignItems: 'center',
    backgroundColor: '#FFFFFF'
  },
  buttonOnline: {
    marginBottom: 20,
    width: 250,
    height: 50,
    alignItems: 'center',
    backgroundColor: '#FFFFFF'
  },
  buttonOnlineText: {
    padding: 15,
    fontSize: 16,
    color: 'green'
  },
  buttonOfflineText: {
    padding: 15,
    fontSize: 16,
    color: 'red'
  },
  introText: {
    paddingBottom: 20,
    fontSize: 14,
    color: 'black'
  },
  endText: {
    paddingBottom: 20,
    fontSize: 14,
    color: 'black'
  },
  imgContainer: {
    height: 400,
    width: 500,
    alignItems: 'center',
    backgroundColor: '#FFFFFF'
  },
  imageHeader: {
    height:65,
    width: 250,
    resizeMode: 'contain',
    alignItems: 'center'
  },
  imageLock: {
    marginTop: 20,
    height: 400,
    width: 300,
    resizeMode: 'contain',
    alignItems: 'center'
  }
})
