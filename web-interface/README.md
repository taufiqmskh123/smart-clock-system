# Smart Exam Controller - Web Interface

A modern, real-time exam alarm management system using Firebase Realtime Database and a sleek dark-themed UI.

## Features

✅ **Hour Input** - 24-hour format (0-23)
✅ **Minute Input** - Standard minutes (0-59)
✅ **Subject Name** - Store exam subject information
✅ **Firebase Integration** - Real-time database updates
✅ **Dark Modern Theme** - Professional cyberpunk-style UI
✅ **Input Validation** - Client-side validation with helpful feedback
✅ **Responsive Design** - Works on desktop and mobile
✅ **Status Feedback** - Real-time success/error messages
✅ **Firebase v9 SDK** - Latest modular SDK

## Setup Instructions

### 1. Get Firebase Credentials
- Go to [Firebase Console](https://console.firebase.google.com/)
- Create a new project or select existing one
- Go to **Project Settings** → **General**
- Copy your Firebase config object

### 2. Update Configuration
In `index.html`, find the Firebase config section and replace the placeholder values:

```javascript
const firebaseConfig = {
    apiKey: "YOUR_API_KEY",
    authDomain: "YOUR_PROJECT.firebaseapp.com",
    projectId: "YOUR_PROJECT_ID",
    storageBucket: "YOUR_PROJECT.appspot.com",
    messagingSenderId: "YOUR_SENDER_ID",
    appId: "YOUR_APP_ID"
};
```

### 3. Configure Firebase Realtime Database
- In Firebase Console, go to **Realtime Database**
- Create a new database
- Set security rules to allow write access:

```json
{
  "rules": {
    "alarm": {
      ".write": true,
      ".read": true
    }
  }
}
```

**Note:** For production, use proper authentication instead of `true` for write access.

### 4. Open in Browser
- Simply open `index.html` in a modern web browser
- No build process or npm installation required!

## Usage

1. **Set Hour** - Enter hour in 24-hour format (0-23)
2. **Set Minute** - Enter minute (0-59)
3. **Enter Subject** - Type the exam subject name
4. **Click "Set Alarm"** - Updates Firebase `/alarm` path with:
   ```
   {
     "active": true,
     "time": "HH:MM",
     "subject": "Subject Name",
     "timestamp": "ISO timestamp"
   }
   ```
5. **See Confirmation** - Success message confirms the alarm was set

## Data Structure

After setting an alarm, your Firebase Realtime Database will have:

```
/
└── alarm/
    ├── active: true
    ├── time: "14:30"
    ├── subject: "Mathematics"
    └── timestamp: "2026-04-18T10:30:00.000Z"
```

## Customization

### Change Theme Colors
Edit the CSS variables in `index.html` style section:
- Primary cyan: `#00d4ff`
- Success green: `#00d400`
- Error red: `#ff3232`
- Background gradients: Update `linear-gradient` values

### Add More Fields
To add more input fields:
1. Add HTML input element
2. Get element by ID with `document.getElementById()`
3. Include in validation logic
4. Add to `alarmData` object before `set()`

## Browser Compatibility

- ✅ Chrome/Brave (latest)
- ✅ Firefox (latest)
- ✅ Safari (latest)
- ✅ Edge (latest)
- ✅ Mobile browsers

## Troubleshooting

### "Firebase Config Error"
- Check your Firebase config values
- Ensure all fields are filled correctly
- Verify project ID matches your Firebase project

### "Database write failed"
- Check Firebase Realtime Database security rules
- Verify database is created in your project
- Check browser console for detailed errors

### "Active status won't update"
- Clear browser cache
- Check database security rules
- Verify `/alarm` path exists in database

## File Structure

```
web-interface/
├── index.html          # Main application (all-in-one)
├── README.md          # This file
└── firebase-config-template.txt  # Config template
```

## Security Notes

⚠️ **For Development Only**: Current rules allow anyone to write to the database.

For production, implement:
- Firebase Authentication
- Proper security rules with user validation
- HTTPS only
- Rate limiting
- Input sanitization

Example production rule:
```json
{
  "rules": {
    "alarm": {
      ".write": "auth.uid != null",
      ".read": "auth.uid != null"
    }
  }
}
```

## Support

- Check browser console (F12) for detailed error messages
- Verify Firebase project connectivity
- Test with Firebase Console's real-time database viewer

Enjoy your Smart Exam Controller! 🚀
