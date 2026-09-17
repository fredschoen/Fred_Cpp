<?php
// Remplace par ton adresse e-mail Free
$to = "schoen.frederic@free.fr";

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