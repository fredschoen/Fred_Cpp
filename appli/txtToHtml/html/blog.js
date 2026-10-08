const blogPages = [
  { pageRef: "mm60802_bonjour.html", previousPageRef: "yyyyyyyy_yy_yy.html", documentNumber: 1, documentCount: 4, nextPageRef: "mm60803_au_revoir.html" },
  { pageRef: "mm60803_au_revoir.html", previousPageRef: "mm60802_bonjour.html", documentNumber: 2, documentCount: 4, nextPageRef: "xxxxxxxx_xx_xx.html" },
  { pageRef: "xxxxxxxx_xx_xx.html", previousPageRef: "mm60803_au_revoir.html", documentNumber: 3, documentCount: 4, nextPageRef: "yyyyyyyy_yy_yy.html" },
  { pageRef: "yyyyyyyy_yy_yy.html", previousPageRef: "xxxxxxxx_xx_xx.html", documentNumber: 4, documentCount: 4, nextPageRef: "mm60802_bonjour.html" },
];

const pageRef = decodeURIComponent(window.location.pathname.split('/').pop());
const blogPage = blogPages.find(page => page.pageRef === pageRef);
if (blogPage) {
    document.querySelectorAll('.previous-page').forEach(link => {
        link.setAttribute('href', encodeURIComponent(blogPage.previousPageRef));
    });
    document.querySelectorAll('.document-counter').forEach(counter => {
        counter.textContent = `${blogPage.documentNumber}/${blogPage.documentCount}`;
    });
    document.querySelectorAll('.next-page').forEach(link => {
        link.setAttribute('href', encodeURIComponent(blogPage.nextPageRef));
    });
}
