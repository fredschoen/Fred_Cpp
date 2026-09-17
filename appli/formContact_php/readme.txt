---
Voici une page HTML simple pour un formulaire de contact, conçue pour être hébergée sur **les Pages Perso Free**. Cette page permet à un visiteur de saisir un message et de l'envoyer par e-mail au propriétaire du site via un script PHP basique (que Free supporte).
---

### **1. Structure de la page HTML (`index.html`)**
Voici le code pour la page `contact.html` (ou `index.html` si c'est la page principale) :

```html
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Contact - Mon Site Perso</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            margin: 0;
            padding: 20px;
            background-color: #f4f4f4;
            color: #333;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background: #fff;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }
        h1 {
            text-align: center;
            color: #444;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
        }
        input[type="text"],
        input[type="email"],
        textarea {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        textarea {
            height: 150px;
        }
        button {
            background: #5cb85c;
            color: #fff;
            border: none;
            padding: 10px 20px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
        }
        button:hover {
            background: #4cae4c;
        }
        .success {
            color: green;
            text-align: center;
            margin-top: 20px;
        }
        .error {
            color: red;
            text-align: center;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Contactez-moi</h1>
        <form action="send_mail.php" method="POST">
            <div class="form-group">
                <label for="name">Nom :</label>
                <input type="text" id="name" name="name" required>
            </div>
            <div class="form-group">
                <label for="email">E-mail :</label>
                <input type="email" id="email" name="email" required>
            </div>
            <div class="form-group">
                <label for="subject">Sujet :</label>
                <input type="text" id="subject" name="subject" required>
            </div>
            <div class="form-group">
                <label for="message">Message :</label>
                <textarea id="message" name="message" required></textarea>
            </div>
            <button type="submit">Envoyer</button>
        </form>
        <?php
        // Afficher un message de succès ou d'erreur si redirigé depuis send_mail.php
        if (isset($_GET['success']) && $_GET['success'] == 1) {
            echo '<div class="success">Votre message a été envoyé avec succès !</div>';
        }
        if (isset($_GET['error']) && $_GET['error'] == 1) {
            echo '<div class="error">Une erreur est survenue. Veuillez réessayer.</div>';
        }
        ?>
    </div>
</body>
</html>
```

---

### **2. Script PHP pour envoyer l'e-mail (`send_mail.php`)**
Free prend en charge PHP, donc tu peux utiliser ce script pour envoyer l'e-mail. Place-le dans le même dossier que ta page HTML.

```php
<?php
// Remplace par ton adresse e-mail Free
$to = "ton_adresse@free.fr";

// Récupération des données du formulaire
$name = htmlspecialchars($_POST['name']);
$email = htmlspecialchars($_POST['email']);
$subject = htmlspecialchars($_POST['subject']);
$message = htmlspecialchars($_POST['message']);

// En-têtes de l'e-mail
$headers = "From: $email\r\n";
$headers .= "Reply-To: $email\r\n";
$headers .= "Content-Type: text/plain; charset=UTF-8\r\n";

// Corps de l'e-mail
$body = "Nom : $name\n";
$body .= "E-mail : $email\n";
$body .= "Sujet : $subject\n\n";
$body .= "Message :\n$message\n";

// Envoi de l'e-mail
if (mail($to, $subject, $body, $headers)) {
    // Redirection vers la page de contact avec un message de succès
    header("Location: contact.html?error=0");
} else {
    // Redirection vers la page de contact avec un message d'erreur
    header("Location: contact.html?error=1");
}
?>
```

---

### **3. Instructions pour héberger sur Free**
1. **Crée un compte Free** (si ce n'est pas déjà fait) et active les **Pages Perso**.
2. **Connecte-toi en FTP** à ton espace perso Free avec un client comme [FileZilla](https://filezilla-project.org/).
   - Serveur : `ftpperso.free.fr`
   - Identifiant : ton identifiant Free (ex: `tonidentifiant`)
   - Mot de passe : ton mot de passe Free
3. **Télécharge les fichiers** `contact.html` et `send_mail.php` dans le dossier `/www/` de ton espace perso.
4. **Teste ton formulaire** en accédant à `http://tonidentifiant.free.fr/contact.html`.

---

### **4. Remarques importantes**
- **Sécurité** : Le script PHP est basique. Pour éviter les abus (spam), tu peux ajouter une vérification CAPTCHA ou limiter les envois.
- **Adresse e-mail** : Remplace `ton_adresse@free.fr` par ton adresse e-mail Free dans `send_mail.php`.
- **Champs requis** : Tous les champs du formulaire sont obligatoires (`required`).
- **Redirections** : Le script PHP redirige vers `contact.html` avec un paramètre `success` ou `error` pour afficher un message.

---
### **5. Alternative sans PHP (si Free bloque `mail()`)**
Si Free bloque la fonction `mail()`, tu peux utiliser un **service externe** comme [Formspree](https://formspree.io/) ou [FormSubmit](https://formsubmit.co/).
Exemple avec Formspree :
- Dans le formulaire HTML, change l'attribut `action` :
  ```html
  <form action="https://formspree.io/f/TA_CLEF" method="POST">
  ```
- Inscris-toi sur Formspree pour obtenir ta clé.

---
Veux-tu que je t'aide à personnaliser cette page ou à ajouter des fonctionnalités (ex: CAPTCHA, champs supplémentaires) ?